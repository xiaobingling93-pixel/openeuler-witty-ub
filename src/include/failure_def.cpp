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

#include "failure_def.h"

#include "utils.h"

namespace failure {
    bool FailureEventQuery::Match(const FailureMetadata& metadata, bool podMode) const
    {
        if (!eventTypes.empty() && eventTypes.find(metadata.eventType) == eventTypes.end()) {
            return false;
        }
        if (podMode && !podIds.empty() && metadata.podId && podIds.find(*metadata.podId) == podIds.end()) {
            return false;
        }
        if (!localEids.empty() && localEids.find(metadata.localEid) == localEids.end()) {
            return false;
        }
        if (!jettyIds.empty() && jettyIds.find(metadata.localJettyId) == jettyIds.end()) {
            return false;
        }
        return true;
    }

    std::optional<EventTypeOption> EventTypeOptionFromString(const std::string& str)
    {
        if (str == "bind") {
            return EventTypeOption::BIND;
        }
        else if (str == "unbind") {
            return EventTypeOption::UNBIND;
        }
        else if (str == "post") {
            return EventTypeOption::POST;
        }
        else {
            return std::nullopt;
        }
    }

    std::string EventTypeOptionToString(EventTypeOption opt)
    {
        switch (opt) {
        case EventTypeOption::BIND: return "bind";
        case EventTypeOption::POST: return "post";
        case EventTypeOption::UNBIND: return "unbind";
        default: return "unknown";
        }
    }

    FailureMode FailureMode::FromJson(const Json::Value& j)
    {
        FailureMode mode;
        mode.component = j["component"].asString();
        mode.version = j["version"].asString();
        mode.isMultiline = j["is_multiline"].asBool();
        mode.manifest = j["manifest"].asString();
        std::vector<PathCell> pathCells = {
            { std::nullopt, j["log_path"].asString() }
        };
        DataSourceOption opt;
        if (mode.component == "hardware" || mode.component == "urmacore" || mode.component == "udmacore") {
            opt = DataSourceOption::KERNEL;
        }
        else {
            opt = DataSourceOption::USER;
        }
        mode.dataSource = { opt, std::move(pathCells) };
        return mode;
    }

    Json::Value FailureEvent::ToJson() const
    {
        Json::Value j(Json::objectValue);
        if (auto time = utils::TimestampToDatetimeStr(timestamp)) {
            j["time"] = *time;
        }
        else {
            j["time"] = Json::nullValue;
        }
        j["component"] = component;
        j["path"] = pathCell.path;
        j["text"] = text;
        j["attributes"] = Json::Value(Json::objectValue);
        if (attributes.find("function_name") != attributes.end()) {
            j["attributes"]["function_name"] = attributes.at("function_name");
        }
        if (attributes.find("local_eid") != attributes.end()) {
            j["attributes"]["local_eid"] = attributes.at("local_eid");
        }
        if (attributes.find("local_jetty_id") != attributes.end()) {
            j["attributes"]["local_jetty_id"] = attributes.at("local_jetty_id");
        }
        if (attributes.find("remote_eid") != attributes.end()) {
            j["attributes"]["remote_eid"] = attributes.at("remote_eid");
        }
        if (attributes.find("remote_jetty_id") != attributes.end()) {
            j["attributes"]["remote_jetty_id"] = attributes.at("remote_jetty_id");
        }
        return j;
    }

    Json::Value FailureMetadata::ToJson() const
    {
        Json::Value j(Json::objectValue);
        j["event_type"] = EventTypeOptionToString(eventType);
        if (podId) {
            j["pod_id"] = *podId;
        }
        else {
            j["pod_id"] = Json::nullValue;
        }
        j["program_name"] = programName;
        j["proc_id"] = procId;
        if (auto time = utils::TimestampToDatetimeStr(timestamp)) {
            j["time"] = *time;
        }
        else {
            j["time"] = Json::nullValue;
        }
        j["local_eid"] = localEid;
        j["local_jetty_id"] = localJettyId;
        if (remoteEid) {
            j["remote_eid"] = *remoteEid;
        }
        else {
            j["remote_eid"] = Json::nullValue;
        }
        if (remoteJettyId) {
            j["remote_jetty_id"] = *remoteJettyId;
        }
        else {
            j["remote_jetty_id"] = Json::nullValue;
        }
        j["text"] = text;
        if (role) {
            j["role"] = *role;
        }
        else {
            j["role"] = Json::nullValue;
        }
        j["logs"] = Json::Value(Json::arrayValue);
        for (const FailureEvent& event : events) {
            j["logs"].append(event.ToJson());
        }
        return j;
    }

    PathCell::PathCell(const std::optional<std::string>& podId, const std::string& path)
        : podId(podId)
        , path(path)
    {
    }
}