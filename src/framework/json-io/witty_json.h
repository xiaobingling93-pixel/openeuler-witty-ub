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
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <filesystem>
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
            
            // Generate temporary file path
            std::filesystem::path file_path(filename);
            std::filesystem::path temp_path = file_path.parent_path() / (file_path.stem().string() + ".tmp" + file_path.extension().string());
            std::string temp_filename = temp_path.string();
            
            // Create a dedicated lock file path
            std::string lock_filename = filename + ".lock";
            
            // Acquire file lock on the dedicated lock file
            int lock_fd = open(lock_filename.c_str(), O_WRONLY | O_CREAT, 0644);
            if (lock_fd == -1) {
                LOG_ERROR << "WittyJson::WriteVectorsTofile-Error: failed to open lock file " << lock_filename;
                return RACK_FAIL;
            }
            
            // Wait for exclusive lock (blocking)
            if (flock(lock_fd, LOCK_EX) == -1) {
                LOG_ERROR << "WittyJson::WriteVectorsTofile-Error: failed to acquire lock on " << lock_filename;
                close(lock_fd);
                return RACK_FAIL;
            }
            
            LOG_DEBUG << "WittyJson::WriteVectorsTofile-Info: acquired lock on " << lock_filename;
            
            // Open temporary file for writing
            std::ofstream output(temp_filename);
            if(!output.is_open()){
                LOG_ERROR << "WittyJson::WriteVectorsTofile-Error: failed to open temporary file " << temp_filename;
                flock(lock_fd, LOCK_UN);
                close(lock_fd);
                return RACK_FAIL;
            }
            
            // Write JSON data to temporary file
            Json::StreamWriterBuilder builder;
            builder["indentation"] = "    ";
            std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
            writer->write(j, &output);
            output << std::endl;
            output.close();
            
            // Atomically rename temporary file to target file
            try {
                std::filesystem::rename(temp_filename, filename);
                LOG_INFO << "WittyJson::WriteVectorsTofile-Info: successfully write to file " << filename;
            } catch (const std::filesystem::filesystem_error& e) {
                LOG_ERROR << "WittyJson::WriteVectorsTofile-Error: failed to rename temporary file to " << filename 
                          << ", error: " << e.what();
                flock(lock_fd, LOCK_UN);
                close(lock_fd);
                std::filesystem::remove(temp_filename);
                return RACK_FAIL;
            }
            
            // Release file lock
            if (flock(lock_fd, LOCK_UN) == -1) {
                LOG_ERROR << "WittyJson::WriteVectorsTofile-Error: failed to release lock on " << lock_filename;
            }
            close(lock_fd);
            
            // Remove the lock file (optional, but keeps filesystem clean)
            std::filesystem::remove(lock_filename);
            
            LOG_DEBUG << "WittyJson::WriteVectorsTofile-Info: released lock and cleaned up lock file";
            
            return RACK_OK;
        }
    };
}//namespace witty_json::io
#endif // WITTY_JSON_H