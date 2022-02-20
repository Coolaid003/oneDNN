/*******************************************************************************
* Copyright 2019-2022 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sstream>

#include "dnnl_common.hpp"
#include "dnnl_memory.hpp"
#include "utils/parser.hpp"

#include "resampling/graph_resampling.hpp"
#include "resampling/resampling.hpp"

namespace resampling {

void check_correctness(const settings_t &s) {
    for_(const auto &i_dir : s.dir)
    for_(const auto &i_sdt : s.sdt)
    for_(const auto &i_ddt : s.ddt)
    for_(const auto &i_tag : s.tag)
    for_(const auto &i_alg : s.alg)
    for_(const auto &i_post_ops : s.post_ops)
    for_(const auto &i_mb : s.mb)
    for (const auto &i_scratchpad_mode : s.scratchpad_mode) {
        attr_t attr;
        attr.insert(i_post_ops);
        attr.insert(i_scratchpad_mode);

        const prb_t prb(s.desc, i_dir, i_sdt, i_ddt, i_tag, i_alg, attr, i_mb);
        std::stringstream ss;
        ss << prb;
        const std::string cpp_pstr = ss.str();
        const char *pstr = cpp_pstr.c_str();
        BENCHDNN_PRINT(1, "run: %s\n", pstr);

        res_t res {};
        const int status = [&prb, &res](api_mode_t mode) {
            if (api_mode == PRIMITIVE)
                return doit(&prb, &res);
            else if (api_mode == GRAPH)
                return benchdnnext::resampling::doit(&prb, &res);
            else
                return FAIL;
        }(api_mode);

        bool want_perf_report = false;
        parse_result(res, want_perf_report, status, pstr);

        if (want_perf_report && is_bench_mode(PERF)) {
            perf_report_t pr(&prb, s.perf_template);
            pr.report(&res, pstr);
        }

        benchdnn_stat.tests++;
    }
}

int bench(int argc, char **argv) {
    driver_name = "resampling";
    using namespace parser;
    static settings_t s;
    static const settings_t def {};
    for (; argc > 0; --argc, ++argv) {
        const bool parsed_options = parse_bench_settings(argv[0])
                || parse_batch(bench, argv[0])
                || parse_dir(s.dir, def.dir, argv[0])
                || parse_dt(s.sdt, def.sdt, argv[0], "sdt")
                || parse_dt(s.ddt, def.ddt, argv[0], "ddt")
                || parse_tag(s.tag, def.tag, argv[0])
                || parse_alg(s.alg, def.alg, str2alg, argv[0])
                || parse_mb(s.mb, def.mb, argv[0])
                || parse_attr_post_ops(s.post_ops, argv[0])
                || parse_attr_scratchpad_mode(
                        s.scratchpad_mode, def.scratchpad_mode, argv[0])
                || parse_perf_template(s.perf_template, s.perf_template_def,
                        s.perf_template_csv(), argv[0])
                || parse_reset(s, argv[0]) || parse_help(argv[0]);
        if (!parsed_options) {
            catch_unknown_options(argv[0]);

            SAFE(str2desc(&s.desc, argv[0]), CRIT);
            check_correctness(s);
        }
    }

    return parse_last_argument();
}

} // namespace resampling
