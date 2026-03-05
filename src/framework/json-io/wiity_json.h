#ifndef WITTY_JSON_H
#define WITTY_JSON_H
#include <fstream>
#include "nlohmann/json.hpp"
#include "logger.h"
#include "rack_error.h"
namespace witty_json::io {
    using json = nlohmann::json;
    Class WITTY_JSON_H{
        public:
        void add_vector_to_json(json &j){
        }
        template<typename T, typename...Rest>
        void add_vector_to_json(json &j, std::pair<const char *, const std::vector<T> &> &first, Rest &&...rest){
            j[v.first] = v.second;
            add_vector_to_json(j, std::forward<Rest>(rest)...);
        }
        template<typename...Pairs>
        RackResult WriteVectorsTofile(const std::string &filename, Pairs &&...pairs){
            json j;
            add_vector_to_json(j, std::forward<Pairs>(pairs)...);
            std::ofstream input(filename);
            if(!file.is_open()){
                LOG_ERROR << "WittyJson::WriteVectorsTofile-Error: failed to open file " << filename;
                return RACK_FAIL;
            }
            input << j.dump(4) << std::endl;
            input.close();
            LOG_INFO << "WittyJson::WriteVectorsTofile-Info: successfully write to file " << filename;
            return RACK_OK;
        }
    };
}//namespace witty_json::io
#endif // WITTY_JSON_H
