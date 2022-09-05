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


} // namespace dxpool
#endif