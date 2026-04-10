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

#define MODULE_NAME "LOG"

#include "log_callstack_collector.h"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <thread>
#include <vector>

#include "failure_def.h"
#include "http/rack_http_client.h"
#include "logger.h"
#include "ubse_context.h"

namespace failure::log {
using namespace ubse::context;
using namespace rack::com;

constexpr const char *ANALYSIS_SKILL_NAME = "ub-callstack-analysis";
constexpr const char *AGGREGATION_SKILL_NAME = "ub-callstack-aggregation";
constexpr const char *KEYFUNC_SKILL_NAME = "ub-keyfunc-analysis";
constexpr const char *DIAG_ANALYSIS_ROOT_DIR = "/var/witty-ub/callstack-analysis";
constexpr const char *OPENCODE_URL = "http://127.0.0.1";
constexpr const char *OPENCODE_PORT = "4096";
constexpr const char *OPENCODE_CONTENT_TYPE = "application/json";
constexpr const char *OPENCODE_PATH_SESSION = "/session";
constexpr const char *SESSION_ID_PLACEHOLDER = ":id";
constexpr const char *OPENCODE_PATH_SESSION_MESSAGE = "/session/:id/message";
constexpr const char *OPENCODE_PATH_SESSION_PROMPT_ASYNC = "/session/:id/prompt_async";
constexpr const char *OPENCODE_BASIC_AUTH_DEFAULT_USERNAME = "opencode";
constexpr const char *BASE64_TABLE = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
constexpr int OPENCODE_PROMPT_ASYNC_SUCCESS = 204;
constexpr int SESSION_POLL_INTERVAL_MS = 2000;
constexpr int SESSION_PROGRESS_LOG_INTERVAL_MS = 5000;
constexpr int SESSION_VISIBLE_TIMEOUT_SEC = 5;
constexpr int SESSION_POLL_TIMEOUT_SEC = 600;
constexpr int INT_2 = 2;
constexpr int INT_3 = 3;
constexpr int INT_4 = 4;
constexpr int INT_6 = 6;
constexpr int INT_8 = 8;
constexpr int INT_12 = 12;
constexpr int INT_16 = 16;
constexpr int INT_18 = 18;

const std::vector<std::string> LogCallstackCollector::allComponents_ = {"ubsocket", "umq", "liburma", "libudma"};

Json::Int64 GetCurrentTimeMs()
{
    return static_cast<Json::Int64>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());
}

Json::Int64 GetMessageCreatedTimeMs(const Json::Value &message)
{
    const Json::Value &info = message["info"];
    const Json::Value &time = info["time"];
    if (!time.isObject() || !time.isMember("created")) {
        return 0;
    }
    return time["created"].asInt64();
}

const Json::Value *FindPreferredAssistantMessage(const Json::Value &messages, Json::Int64 promptSubmittedAtMs)
{
    const Json::Value *latestAssistantMessage = nullptr;
    Json::Int64 latestAssistantCreatedAt = 0;
    const Json::Value *latestCurrentReply = nullptr;
    Json::Int64 latestCurrentReplyCreatedAt = 0;

    for (Json::ArrayIndex i = 0; i < messages.size(); ++i) {
        const Json::Value &message = messages[i];
        const Json::Value &info = message["info"];
        if (info["role"].asString() != "assistant") {
            continue;
        }

        const Json::Int64 createdAt = GetMessageCreatedTimeMs(message);
        if (latestAssistantMessage == nullptr || createdAt >= latestAssistantCreatedAt) {
            latestAssistantMessage = &message;
            latestAssistantCreatedAt = createdAt;
        }
        if (createdAt >= promptSubmittedAtMs &&
            (latestCurrentReply == nullptr || createdAt >= latestCurrentReplyCreatedAt)) {
            latestCurrentReply = &message;
            latestCurrentReplyCreatedAt = createdAt;
        }
    }

    return latestCurrentReply != nullptr ? latestCurrentReply : latestAssistantMessage;
}

std::string BuildSessionPath(const std::string &pathTemplate, const std::string &sessionId)
{
    std::string path = pathTemplate;
    auto pos = path.find(SESSION_ID_PLACEHOLDER);
    if (pos != std::string::npos) {
        path.replace(pos, std::strlen(SESSION_ID_PLACEHOLDER), sessionId);
    }
    return path;
}

std::string Base64Encode(const std::string &input)
{
    std::string out;
    out.reserve(((input.size() + INT_2) / INT_3) * INT_4);

    size_t i = 0;
    for (; i + INT_2 < input.size(); i += INT_3) {
        const uint32_t chunk = (static_cast<uint32_t>(static_cast<unsigned char>(input[i])) << INT_16) |
                               (static_cast<uint32_t>(static_cast<unsigned char>(input[i + 1])) << INT_8) |
                               static_cast<uint32_t>(static_cast<unsigned char>(input[i + 2]));
        out.push_back(BASE64_TABLE[(chunk >> INT_18) & 0x3F]);
        out.push_back(BASE64_TABLE[(chunk >> INT_12) & 0x3F]);
        out.push_back(BASE64_TABLE[(chunk >> INT_6) & 0x3F]);
        out.push_back(BASE64_TABLE[chunk & 0x3F]);
    }

    if (i < input.size()) {
        uint32_t chunk = static_cast<uint32_t>(static_cast<unsigned char>(input[i])) << INT_16;
        out.push_back(BASE64_TABLE[(chunk >> INT_18) & 0x3F]);
        if (i + 1 < input.size()) {
            chunk |= static_cast<uint32_t>(static_cast<unsigned char>(input[i + 1])) << INT_8;
            out.push_back(BASE64_TABLE[(chunk >> INT_12) & 0x3F]);
            out.push_back(BASE64_TABLE[(chunk >> INT_6) & 0x3F]);
            out.push_back('=');
        } else {
            out.push_back(BASE64_TABLE[(chunk >> INT_12) & 0x3F]);
            out.push_back('=');
            out.push_back('=');
        }
    }

    return out;
}

void ApplyBasicAuthHeader(RackHttpRequest &req, const failure::OpencodeConnection &conn)
{
    if (!conn.passwd.has_value() || conn.passwd->empty()) {
        return;
    }

    const std::string username = (conn.username.has_value() && !conn.username->empty()) ?
                                     conn.username.value() :
                                     OPENCODE_BASIC_AUTH_DEFAULT_USERNAME;
    const std::string userAndPassword = username + ":" + conn.passwd.value();
    req.headers["Authorization"] = "Basic " + Base64Encode(userAndPassword);
}

bool StringToJson(Json::Value &root, const std::string &jsonStr)
{
    Json::CharReaderBuilder builder;
    std::string errs;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    bool res = reader->parse(jsonStr.c_str(), jsonStr.c_str() + jsonStr.size(), &root, &errs);
    if (!res) {
        LOG_ERROR << "failed to parse string to json: " << jsonStr;
    }
    return res;
}

std::string LogCallstackCollector::BuildAnalysisSkillPrompt(const std::string &skill, const SkillInput &input,
                                                            const std::string &component)
{
    std::string prompt = "请使用 " + skill + " skill 来完成源码分析。\n";
    prompt += "请严格按该 skill 的输入输出要求执行，并生成结果文件。\n";
    const auto it = input.componentsPaths.find(component);
    if (it == input.componentsPaths.end()) {
        return "";
    }
    prompt += "请仅分析以下单个组件：\n";
    prompt += "- component: " + component + "\n";
    prompt += "- source_path: " + it->second + "\n";
    const auto compileIt = input.compileCommandsPaths.find(component);
    if (compileIt != input.compileCommandsPaths.end()) {
        prompt += "- compile_commands_path: " + compileIt->second + "\n";
    }
    return prompt;
}

std::string LogCallstackCollector::BuildAggregationSkillPrompt(const std::string &skill, const SkillInput &input)
{
    std::string prompt = "请使用 " + skill + " skill 来完成调用图聚合。\n";
    prompt += "请严格按该 skill 的输入输出要求执行，并生成结果文件。\n";
    prompt += "以下为聚合必需输入：\n";
    for (const auto &component : allComponents_) {
        const auto it = input.componentsPaths.find(component);
        if (it == input.componentsPaths.end()) {
            return "";
        }
        prompt += "- --" + component + "-src: " + it->second + "\n";
        prompt += "- --" + component + "-callstack: " + std::string(DIAG_ANALYSIS_ROOT_DIR) + "/" + component +
                  "/callstack.json\n";
    }
    prompt += "- 输出目录: " + std::string(DIAG_ANALYSIS_ROOT_DIR) + "\n";
    prompt += "- 输出文件: overall_callstack.json\n";
    return prompt;
}

std::string LogCallstackCollector::BuildKeyfuncSkillPrompt(const std::string &skill, const SkillInput &input)
{
    constexpr const char *keyfuncComponent = "umq"; // 当前默认network模式，非network下分析urma

    std::string prompt = "请使用 " + skill + " skill 来完成关键函数分析。\n";
    prompt += "请严格按该 skill 的输入输出要求执行，并生成结果文件。\n";

    const auto it = input.componentsPaths.find(keyfuncComponent);
    if (it == input.componentsPaths.end()) {
        return "";
    }

    prompt += "以下为关键函数分析输入：\n";
    prompt += "- component: " + std::string(keyfuncComponent) + "\n";
    prompt += "- source_path: " + it->second + "\n";
    prompt += "- keywords: bind,unbind,post,poll\n";
    return prompt;
}

RackResult LogCallstackCollector::Initialize()
{
    RackResult ret = RACK_OK;
    if ((ret = ParseArgs()) != RACK_OK) {
        LOG_ERROR << "failed to parse arguments";
        return ret;
    }
    return RACK_OK;
}

void LogCallstackCollector::UnInitialize() {}

RackResult LogCallstackCollector::Start()
{
    for (const std::string &component : allComponents_) {
        const auto it = input_.componentsPaths.find(component);
        if (it == input_.componentsPaths.end()) {
            LOG_ERROR << "missing source path for component: " << component;
            return RACK_FAIL;
        }
    }

    for (const std::string &component : allComponents_) {
        const std::string analysisPrompt = BuildAnalysisSkillPrompt(ANALYSIS_SKILL_NAME, input_, component);
        if (analysisPrompt.empty()) {
            LOG_ERROR << "failed to build analysis prompt for component: " << component;
            return RACK_FAIL;
        }

        RackResult ret = Run(ANALYSIS_SKILL_NAME, analysisPrompt);
        if (ret != RACK_OK) {
            LOG_ERROR << "failed to run analysis skill for component: " << component;
            return ret;
        }
    }

    const std::string aggregationPrompt = BuildAggregationSkillPrompt(AGGREGATION_SKILL_NAME, input_);
    if (aggregationPrompt.empty()) {
        LOG_ERROR << "failed to build aggregation prompt";
        return RACK_FAIL;
    }

    RackResult ret = Run(AGGREGATION_SKILL_NAME, aggregationPrompt);
    if (ret != RACK_OK) {
        LOG_ERROR << "failed to run aggregation skill";
        return ret;
    }

    const std::string keyfuncPrompt = BuildKeyfuncSkillPrompt(KEYFUNC_SKILL_NAME, input_);
    if (keyfuncPrompt.empty()) {
        LOG_ERROR << "failed to build keyfunc prompt";
        return RACK_FAIL;
    }

    ret = Run(KEYFUNC_SKILL_NAME, keyfuncPrompt);
    if (ret != RACK_OK) {
        LOG_ERROR << "failed to run keyfunc skill";
        return ret;
    }

    return RACK_OK;
}

void LogCallstackCollector::Stop() {}

RackResult LogCallstackCollector::ParseOpencodeConn(const std::unordered_map<std::string, std::string> &argMap)
{
    auto it = argMap.find("opencode-url");
    if (it == argMap.end()) {
        LOG_ERROR << "missing argument opencode-url";
        return RACK_FAIL;
    }
    conn_.url = it->second;
    it = argMap.find("opencode-port");
    if (it == argMap.end()) {
        LOG_ERROR << "missing argument opencode-port";
        return RACK_FAIL;
    }
    const std::string &port = it->second;
    try {
        conn_.port = std::stoi(port);
    } catch (const std::exception &e) {
        LOG_ERROR << "invalid argument top-n: " << port;
        return RACK_FAIL;
    }
    it = argMap.find("opencode-username");
    if (it == argMap.end() || it->second.empty()) {
        LOG_WARN << "opencode-username is missing or empty, use default username: "
                 << OPENCODE_BASIC_AUTH_DEFAULT_USERNAME;
    } else {
        conn_.username = it->second;
    }
    it = argMap.find("opencode-passwd");
    if (it == argMap.end()) {
        LOG_WARN << "opencode-passwd is missing, please check your input";
    } else {
        if (it->second.empty()) {
            LOG_WARN << "opencode-passwd is empty, please check your input";
        }
        conn_.passwd = it->second;
    }
    return RACK_OK;
}

RackResult LogCallstackCollector::ParseSrcPath(const std::unordered_map<std::string, std::string> &argMap)
{
    auto handleSrcPath = [this, &argMap](const std::string &component) -> RackResult {
        const std::string arg = component + "-src-path";
        const auto it = argMap.find(arg);
        if (it == argMap.end()) {
            LOG_ERROR << "missing argument " << arg;
            return RACK_FAIL;
        }
        input_.componentsPaths[component] = it->second;
        return RACK_OK;
    };

    if (handleSrcPath("ubsocket") != RACK_OK)
        return RACK_FAIL;
    if (handleSrcPath("umq") != RACK_OK)
        return RACK_FAIL;
    if (handleSrcPath("liburma") != RACK_OK)
        return RACK_FAIL;
    if (handleSrcPath("libudma") != RACK_OK)
        return RACK_FAIL;

    return RACK_OK;
}

RackResult LogCallstackCollector::ParseCompileCommandsPath(const std::unordered_map<std::string, std::string> &argMap)
{
    auto handleCompileCommandsPath = [this, &argMap](const std::string &component) {
        const std::string arg = component + "-compile-commands-path";
        const auto explicitIt = argMap.find(arg);
        if (explicitIt != argMap.end()) {
            input_.compileCommandsPaths[component] = explicitIt->second;
            return;
        }

        const auto srcIt = input_.componentsPaths.find(component);
        if (srcIt == input_.componentsPaths.end()) {
            return;
        }

        const std::filesystem::path guessedPath = std::filesystem::path(srcIt->second) / "compile_commands.json";
        if (std::filesystem::exists(guessedPath) && std::filesystem::is_regular_file(guessedPath)) {
            input_.compileCommandsPaths[component] = guessedPath.string();
        }
    };

    handleCompileCommandsPath("ubsocket");
    handleCompileCommandsPath("umq");
    handleCompileCommandsPath("liburma");
    handleCompileCommandsPath("libudma");
    return RACK_OK;
}

RackResult LogCallstackCollector::ParseArgs()
{
    auto &argMap = UbseContext::GetInstance().GetArgMap();

    if (ParseOpencodeConn(argMap) != RACK_OK) {
        LOG_ERROR << "failed to parse source path";
        return RACK_FAIL;
    }
    if (ParseSrcPath(argMap) != RACK_OK) {
        LOG_ERROR << "failed to parse source path";
        return RACK_FAIL;
    }
    if (ParseCompileCommandsPath(argMap) != RACK_OK) {
        LOG_ERROR << "failed to parse compile_commands path";
        return RACK_FAIL;
    }

    return RACK_OK;
}

RackResult LogCallstackCollector::CreateSession()
{
    RackHttpClient client(conn_.url + ":" + std::to_string(conn_.port));
    RackComContext ctx;
    RackHttpRequest req;
    req.method = RackHttpMethod::POST;
    req.path = OPENCODE_PATH_SESSION;
    ApplyBasicAuthHeader(req, conn_);
    auto res = client.Do(ctx, req);
    if (!res.Ok()) {
        LOG_ERROR << "postSession-Error: failed to create session, " << res.message;
        return RACK_FAIL;
    }
    const auto &resp = res.value;
    if (resp->status != OK_200) {
        LOG_ERROR << "postSession-Error: failed to create session, " << res.message;
        return RACK_FAIL;
    }
    Json::Value session;
    if (!StringToJson(session, resp->body)) {
        return RACK_FAIL;
    }
    sessionId_ = session["id"].asString();
    LOG_INFO << "postSession-Info: created session " << sessionId_;
    return RACK_OK;
}

RackResult LogCallstackCollector::WaitForSessionVisible() const
{
    RackHttpClient client(conn_.url + ":" + std::to_string(conn_.port));
    RackComContext ctx;
    RackHttpRequest req;
    req.method = RackHttpMethod::GET;
    req.path = BuildSessionPath(OPENCODE_PATH_SESSION, sessionId_);
    ApplyBasicAuthHeader(req, conn_);

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(SESSION_VISIBLE_TIMEOUT_SEC);

    while (std::chrono::steady_clock::now() < deadline) {
        auto res = client.Do(ctx, req);
        if (!res.Ok()) {
            LOG_INFO << "getSession-Info: session " << sessionId_ << " not queryable yet, error: " << res.message;
            std::this_thread::sleep_for(std::chrono::milliseconds(SESSION_POLL_INTERVAL_MS));
            continue;
        }

        const auto &resp = res.value;
        if (resp->status == OK_200) {
            LOG_INFO << "getSession-Info: session " << sessionId_ << " is queryable";
            return RACK_OK;
        }

        LOG_INFO << "getSession-Info: session " << sessionId_ << " not ready yet, status: " << resp->status
                 << ", body: " << resp->body;
        std::this_thread::sleep_for(std::chrono::milliseconds(SESSION_POLL_INTERVAL_MS));
    }

    LOG_ERROR << "getSession-Error: polling session visibility timed out after " << SESSION_VISIBLE_TIMEOUT_SEC
              << " seconds, session: " << sessionId_;
    return RACK_FAIL;
}

RackResult LogCallstackCollector::SendMessageAsync(const std::string &prompt)
{
    RackHttpClient client(conn_.url + ":" + std::to_string(conn_.port));
    RackComContext ctx;
    RackHttpRequest req;
    req.headers["Content-Type"] = OPENCODE_CONTENT_TYPE;
    req.headers["Accept"] = OPENCODE_CONTENT_TYPE;
    req.method = RackHttpMethod::POST;
    req.path = BuildSessionPath(OPENCODE_PATH_SESSION_PROMPT_ASYNC, sessionId_);
    ApplyBasicAuthHeader(req, conn_);
    Json::Value part(Json::objectValue);
    part["type"] = "text";
    part["text"] = prompt;
    Json::Value body(Json::objectValue);
    body["parts"] = Json::Value(Json::arrayValue);
    body["parts"].append(part);
    req.body = body.toStyledString();
    const Json::Int64 requestStartAtMs = GetCurrentTimeMs();
    auto res = client.Do(ctx, req);
    if (!res.Ok()) {
        LOG_ERROR << "postSessionPromptAsync-Error: failed to send message, " << res.message;
        return RACK_FAIL;
    }
    const auto &resp = res.value;
    if (resp->status != OPENCODE_PROMPT_ASYNC_SUCCESS) {
        LOG_ERROR << "postSessionPromptAsync-Error: failed to send message, "
                  << "status: " << resp->status << ", body: " << resp->body;
        return RACK_FAIL;
    }
    promptSubmittedAtMs_ = requestStartAtMs;
    LOG_INFO << "postSessionPromptAsync-Info: sent async message to session " << sessionId_;
    return RACK_OK;
}

RackResult LogCallstackCollector::WaitForAssistantMessageCompleted() const
{
    RackHttpClient client(conn_.url + ":" + std::to_string(conn_.port));
    RackComContext ctx;
    RackHttpRequest req;
    req.method = RackHttpMethod::GET;
    req.path = BuildSessionPath(OPENCODE_PATH_SESSION_MESSAGE, sessionId_) + "?limit=20";
    ApplyBasicAuthHeader(req, conn_);

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(SESSION_POLL_TIMEOUT_SEC);
    auto lastProgressLogTime =
        std::chrono::steady_clock::now() - std::chrono::milliseconds(SESSION_PROGRESS_LOG_INTERVAL_MS);

    while (std::chrono::steady_clock::now() < deadline) {
        auto res = client.Do(ctx, req);
        if (!res.Ok()) {
            LOG_ERROR << "getSessionMessage-Error: failed to fetch session messages, " << res.message;
            return RACK_FAIL;
        }

        const auto &resp = res.value;
        if (resp->status != OK_200) {
            LOG_ERROR << "getSessionMessage-Error: unexpected status: " << resp->status << ", body: " << resp->body;
            return RACK_FAIL;
        }

        Json::Value messages;
        if (!StringToJson(messages, resp->body)) {
            return RACK_FAIL;
        }
        if (!messages.isArray()) {
            LOG_ERROR << "getSessionMessage-Error: response is not an array";
            return RACK_FAIL;
        }

        const Json::Value *message = FindPreferredAssistantMessage(messages, promptSubmittedAtMs_);
        if (message == nullptr) {
        } else {
            const Json::Value &time = (*message)["info"]["time"];
            const bool completed = time.isObject() && time.isMember("completed");
            if (completed) {
                LOG_INFO << "getSessionMessage-Info: session " << sessionId_ << " assistant message completed";
                return RACK_OK;
            }
        }

        const auto now = std::chrono::steady_clock::now();
        if (now - lastProgressLogTime >= std::chrono::milliseconds(SESSION_PROGRESS_LOG_INTERVAL_MS)) {
            LOG_INFO << "getSessionMessage-Info: session " << sessionId_ << " waiting for assistant message completion";
            lastProgressLogTime = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(SESSION_POLL_INTERVAL_MS));
    }

    LOG_ERROR << "getSessionMessage-Error: polling assistant message completion timed out after "
              << SESSION_POLL_TIMEOUT_SEC << " seconds, session: " << sessionId_;
    return RACK_FAIL;
}

RackResult LogCallstackCollector::FetchLatestAssistantMessage() const
{
    RackHttpClient client(conn_.url + ":" + std::to_string(conn_.port));
    RackComContext ctx;
    RackHttpRequest req;
    req.method = RackHttpMethod::GET;
    req.path = BuildSessionPath(OPENCODE_PATH_SESSION_MESSAGE, sessionId_) + "?limit=20";
    ApplyBasicAuthHeader(req, conn_);

    auto res = client.Do(ctx, req);
    if (!res.Ok()) {
        LOG_ERROR << "getSessionMessage-Error: failed to fetch session messages, " << res.message;
        return RACK_FAIL;
    }

    const auto &resp = res.value;
    if (resp->status != OK_200) {
        LOG_ERROR << "getSessionMessage-Error: unexpected status: " << resp->status << ", body: " << resp->body;
        return RACK_FAIL;
    }

    Json::Value messages;
    if (!StringToJson(messages, resp->body)) {
        return RACK_FAIL;
    }
    if (!messages.isArray()) {
        LOG_ERROR << "getSessionMessage-Error: response is not an array";
        return RACK_FAIL;
    }

    const Json::Value *message = FindPreferredAssistantMessage(messages, promptSubmittedAtMs_);
    if (message == nullptr) {
        LOG_ERROR << "getSessionMessage-Error: no assistant message found in session " << sessionId_;
        return RACK_FAIL;
    }

    const Json::Value &parts = (*message)["parts"];
    std::string text;
    if (parts.isArray()) {
        for (Json::ArrayIndex j = 0; j < parts.size(); ++j) {
            const Json::Value &part = parts[j];
            if (part["type"].asString() != "text") {
                continue;
            }
            if (!text.empty()) {
                text += "\n";
            }
            text += part["text"].asString();
        }
    }

    if (text.empty()) {
        LOG_INFO << "getSessionMessage-Info: selected assistant message has no text part, raw body: "
                 << Json::writeString(Json::StreamWriterBuilder(), *message);
    } else {
        LOG_INFO << "getSessionMessage-Info: selected assistant reply: " << text;
    }
    return RACK_OK;
}

RackResult LogCallstackCollector::Run(const std::string &skill, const std::string &prompt)
{
    LOG_INFO << "run-Info: start skill " << skill;
    RackResult res = RACK_OK;
    if ((res = CreateSession()) != RACK_OK) {
        return res;
    }
    if ((res = WaitForSessionVisible()) != RACK_OK) {
        return res;
    }
    if ((res = SendMessageAsync(prompt)) != RACK_OK) {
        return res;
    }
    if ((res = WaitForAssistantMessageCompleted()) != RACK_OK) {
        return res;
    }
    if ((res = FetchLatestAssistantMessage()) != RACK_OK) {
        return res;
    }
    LOG_INFO << "run-Info: completed skill " << skill;
    return res;
}
} // namespace failure::log
