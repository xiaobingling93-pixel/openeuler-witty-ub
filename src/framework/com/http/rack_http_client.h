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

#include "common/rack_com_context.h"
#include "http/rack_http_client_handler.h"



namespace rack::com {
    class RackHttpClient : public RackHttpClientHandler {
    public:
        explicit RackHttpClient(std::string baseUrl);

        RackComResult<RackHttpResponse> Do(
            const RackComContext& context,
            const RackHttpRequest& request
        ) override;

    private:
        std::string baseUrl_;
    };
}