#ifndef WITTY_JSON_H
#define WITTY_JSON_H
#include <fstream>
#include <json/json.h>
#include <json/writer.h>
#include "logger.h"
#include "rack_error.h"

namespace witty_json::io {
    class WittyJson{
        public:
        void add_vector_to_json(Json::Value &j){
        }
        
        template<typename T, typename...Rest>
        void add_vector_to_json(Json::Value &j, std::pair<const char *, const std::vector<T> &> &first, Rest &&...rest){
            j[first.first] = Json::Value(Json::arrayValue);
            for (const auto &item : first.second) {
                Json::Value item_json;
                to_json(item_json, item);
                j[first.first].append(item_json);
            }
            add_vector_to_json(j, std::forward<Rest>(rest)...);
        }
        
        template<typename...Pairs>
        RackResult WriteVectorsToFile(const std::string &filename, Pairs &&...pairs){
            Json::Value j(Json::objectValue);
            add_vector_to_json(j, std::forward<Pairs>(pairs)...);
            
            std::ofstream output(filename);
            if(!output.is_open()){
                LOG_ERROR << "WittyJson::WriteVectorsTofile-Error: failed to open file " << filename;
                return RACK_FAIL;
            }
            
            Json::StreamWriterBuilder builder;
            builder["indentation"] = "    ";
            std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
            writer->write(j, &output);
            output << std::endl;
            output.close();
            
            LOG_INFO << "WittyJson::WriteVectorsTofile-Info: successfully write to file " << filename;
            return RACK_OK;
        }
    };
}//namespace witty_json::io
#endif // WITTY_JSON_H