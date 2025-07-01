//
//   Copyright 2016 Nvidia
//
//   Licensed under the Apache License, Version 2.0 (the "Apache License")
//   with the following modification; you may not use this file except in
//   compliance with the Apache License and the following modification to it:
//   Section 6. Trademarks. is deleted and replaced with:
//
//   6. Trademarks. This License does not grant permission to use the trade
//      names, trademarks, service marks, or product names of the Licensor
//      and its affiliates, except as required to comply with Section 4(c) of
//      the License and to reproduce the content of the NOTICE file.
//
//   You may obtain a copy of the Apache License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the Apache License with the above modification is
//   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//   KIND, either express or implied. See the Apache License for the specific
//   language governing permissions and limitations under the Apache License.
//

#include "./options.h"
#include "./init_shapes.h"
#include "./regressionTask.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <execution>
#include <filesystem>
#include <sstream>

int _numTasks = 0;

std::atomic<int> _pass = 0, _fail = 0, _knownFail, _completed = 0;

std::array _skipSet = {
    "catmark_car",
    "catmark_bishop",
    "catmark_pawn",
    "catmark_rook",
};

// the following shapes are known to fail - adding them to this list allows the
// regression to flag these tests as 'known failure' and return EXIT_SUCCESS to
// CTest
std::array _knownFailures = {
    "catmark_chaikin1",
    "catmark_lefthanded",
    "catmark_righthanded",
    "loop_chaikin1",
};

static bool isKnownFailure(ShapeDesc const& shape) {

    auto it = std::find_if(_knownFailures.begin(), _knownFailures.end(),
        [&shape](char const* name) { return std::strcmp(shape.name.c_str(), name) == 0; });
    
    return it != _knownFailures.end();
}

struct TasksBatch {

    std::string name = "Quick tests";

    Options options;

    std::vector<RegressionTask> tasks;

    std::atomic<int> pass = 0;
    std::atomic<int> knownFail = 0;
    std::atomic<int> fail = 0;

    void initialize(Options const& opts) {

        options = opts;

        auto const& skipSet = _skipSet;
          
        tasks.reserve(options.shapes.size());

        for (auto const& shape : options.shapes) {

            auto it = std::find_if(skipSet.begin(), skipSet.end(),
                [&shape](char const* skipname) { return std::strncmp(shape.name.data(), skipname, shape.name.size()) == 0; });
        
            if (it != skipSet.end())
                continue;

            bool knownFailure = options.ignoreKnownFailures && isKnownFailure(shape);

            tasks.push_back({ .shapeDesc = &shape, .options = &options, .isKnownFailure = knownFailure });
        }   
    }

    void execute() {

        auto executeTask = [this](RegressionTask& task) {

            if (task.execute() && task.meshDelta.numFacesWithDeltas == 0) {
                ++pass; ++_pass;
            } else {          
                
                if (task.meshDelta.numFacesWithDeltas > 0) {
                    if (task.isKnownFailure) {
                        ++_knownFail; ++knownFail;
                    } else {
                        ++_fail; ++fail;
                    }
                }
            }           
            ++_completed;
            if (options.printProgress) {
                std::fprintf(stdout, "\rpass:%d / known fail:%d / fail:%d (%d/%d)",
                    _pass.load(), _knownFail.load(), _fail.load(), _completed.load(), _numTasks);
                std::fflush(stdout);
            }
        };

        if (options.multi_threaded) {
            std::for_each(std::execution::par_unseq, tasks.begin(), tasks.end(), executeTask);
        } else {
            for (auto& task : tasks)
                executeTask(task);
        }

        std::sort(tasks.begin(), tasks.end(), [](RegressionTask const& a, RegressionTask const& b) {
            return a.meshDelta.numFacesWithDeltas < b.meshDelta.numFacesWithDeltas; });
    }

    void printResults(FILE* f, Options::PrintMask mask) const {

        if (options.printSummary) {
            std::fprintf(f, "Batch %s {\n", name.c_str());

            options.print(f, mask);

            int completed = pass.load() + knownFail.load() + fail.load();

            std::fprintf(f, "\tResults: pass:%d / known fail:%d / fail:%d (%d/%d)\n",
                pass.load(), knownFail.load(), fail.load(), completed, (int)tasks.size());

            if (knownFail.load() > 0 || fail.load() > 0) {
                for (auto const& task : tasks) {
                    if (task.meshDelta.numFacesWithDeltas > 0)
                        task.printMeshDelta();
                }
            }
            else {
                std::fprintf(f, "\t no failures\n");
            }
            std::fprintf(f, "}\n");
        } else {
            if (knownFail.load() > 0 || fail.load() > 0) {
                std::fprintf(f, "Batch: '%s' {\n", name.c_str());
                for (auto const& task : tasks) {
                    if (task.meshDelta.numFacesWithDeltas > 0)
                        task.printMeshDelta();
                }
                std::fprintf(f, "}\n");
            }
        }
    }
};

std::unique_ptr<TasksBatch> createBatchVertex(Options const& opts) {
    auto batch = std::make_unique<TasksBatch>();
    batch->initialize(opts);
    batch->name = "vertex interpolation";
    batch->options.evaluateUV = false;
    batch->options.mayaLogPath /= "vtx_default";
    return batch;
}
std::unique_ptr<TasksBatch> createBatchFVarLinearAll(Options const& opts) {
    auto batch = std::make_unique<TasksBatch>();
    batch->initialize(opts);
    batch->name = "face-varying (bi-linear interpolation)";
    batch->options.ignoreVtx = true;
    batch->options.fvarBoundary = Options::FVarBoundary::kOverride_LinearAll;
    batch->options.mayaLogPath /= "fvar_linear_all";
    return batch;
}
std::unique_ptr<TasksBatch> createBatchFVarLinearNone(Options const& opts) {
    auto batch = std::make_unique<TasksBatch>();
    batch->initialize(opts);
    batch->name = "face-varying (linear edge-only interpolation)";
    batch->options.ignoreVtx = true;
    batch->options.fvarBoundary = Options::FVarBoundary::kOverride_LinearNone;
    batch->options.isolationSmooth = opts.isolationSharp;
    batch->options.mayaLogPath /= "fvar_linear_none";
    return batch;
}
std::unique_ptr<TasksBatch> createBatchFVarLinearCornersOnly(Options const& opts) {
    auto batch = std::make_unique<TasksBatch>();
    batch->initialize(opts);
    batch->name = "face-varying (linear corners-only interpolation)";
    batch->options.ignoreVtx = true;
    batch->options.fvarBoundary = Options::FVarBoundary::kOverride_LinearCornersOnly;
    batch->options.isolationSmooth = opts.isolationSharp;
    batch->options.mayaLogPath /= "fvar_linear_corners_only";
    return batch;
}
std::unique_ptr<TasksBatch> createBatchFVarLinearCornersPlus1(Options const& opts) {
    auto batch = std::make_unique<TasksBatch>();
    batch->initialize(opts);
    batch->name = "face-varying (linear corners-plus1 interpolation)";
    batch->options.ignoreVtx = true;
    batch->options.fvarBoundary = Options::FVarBoundary::kOverride_LinearCornersPlus1;
    batch->options.isolationSmooth = opts.isolationSharp;
    batch->options.mayaLogPath /= "fvar_linear_corners_plus1";
    return batch;
}
std::unique_ptr<TasksBatch> createBatchFVarLinearCornersPlus2(Options const& opts) {
    auto batch = std::make_unique<TasksBatch>();
    batch->initialize(opts);
    batch->name = "face-varying (linear corners-plus2 interpolation)";
    batch->options.ignoreVtx = true;
    batch->options.fvarBoundary = Options::FVarBoundary::kOverride_LinearCornersPlus2;
    batch->options.isolationSmooth = opts.isolationSharp;
    batch->options.mayaLogPath /= "fvar_linear_corners_plus2";
    return batch;
}
std::unique_ptr<TasksBatch> createBatchFVarLinearBoundaries(Options const& opts) {
    auto batch = std::make_unique<TasksBatch>();
    batch->initialize(opts);
    batch->name = "face-varying (linear boundaries interpolation)";
    batch->options.ignoreVtx = true;
    batch->options.fvarBoundary = Options::FVarBoundary::kOverride_LinearBoundaries;
    batch->options.isolationSmooth = opts.isolationSharp;
    batch->options.mayaLogPath /= "fvar_linear_boundaries";
    return batch;
}

uint32_t runStandardBatches(Options const& options) {

    std::array batches = {
        createBatchVertex(options),
        createBatchFVarLinearAll(options),
        createBatchFVarLinearNone(options),
        createBatchFVarLinearCornersOnly(options),
        createBatchFVarLinearCornersPlus1(options),
        createBatchFVarLinearCornersPlus2(options),
        createBatchFVarLinearBoundaries(options),
    };

    for (auto const& batch : batches)
        _numTasks += (int)batch->tasks.size();

    auto executeBatch = [](std::unique_ptr<TasksBatch>& batch) {
        batch->execute();
    };

    if (options.multi_threaded)
        std::for_each(std::execution::par_unseq, batches.begin(), batches.end(), executeBatch);
    else
        for (auto& batch : batches)
            executeBatch(batch);

    using enum Options::PrintMask;

    if (options.printSummary)
        options.print(stdout, Options::PrintMask(kGeneralInfo | kComparisonOptions | kOutputOptions));

    uint32_t failures = 0;
    for (auto const& batch : batches) {
        batch->printResults(stdout, Options::PrintMask(kEvaluationOptions));
        failures += batch->fail.load();
    }
    return failures;
}

uint32_t runSingleBatch(Options const& options) {

    //options.print(stdout);

    Stopwatch time;
    time.Start();

    TasksBatch tests;
    tests.initialize(options);

    _numTasks = (int)tests.tasks.size();

    tests.execute();
    
    tests.printResults(stdout, Options::PrintMask::kAll);

    return tests.fail.load();
}

int main(int argc, char const** argv) {

    Options options;

    try {
        options.initialize(argc, argv);
    } catch (std::invalid_argument const& e) {
        std::fprintf(stderr, "%s\n", e.what());
        options.print(stdout);
        return 0;
    };

    RegressionTask::populateTessCache(options.tessRate);

    Stopwatch time;
    time.Start();

    uint32_t failureCount = 0;
    if (options.fullBatchTesting)
        failureCount = runStandardBatches(options);
    else
        failureCount = runSingleBatch(options);

    time.Stop();

    if (options.printSummary)
        std::fprintf(stdout, "Total time: %f(s)\n\n", time.GetTotalElapsedSeconds());

    return failureCount > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
