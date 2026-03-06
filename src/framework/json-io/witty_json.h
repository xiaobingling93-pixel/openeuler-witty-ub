/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 * witty-ub is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

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