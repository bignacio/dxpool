#ifndef PROCESSOR_H
#define PROCESSOR_H

#ifdef __linux__
#include "ProcessorLinux.h"
#else
#include "ProcessorUnsupported.h"
#endif

namespace dxpool {
#ifdef __linux__
using Processor = dxpool::ProcessorLinux;
#else
using Processor = dxpool::ProcessorUnsupported;
#endif


static inline auto GetAllAvailableCores() -> std::set<Core> {
    Processor processor;
    return processor.FindAvailableCores();
}

} // namespace dxpool
#endif