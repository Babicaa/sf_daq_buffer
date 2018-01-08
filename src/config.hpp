#include <H5Cpp.h>

#ifndef CONFIG_H
#define CONFIG_H

namespace config
{
    extern H5::PredType dataset_type;
    extern H5T_order_t dataset_byte_order;
    extern hsize_t dataset_increase_step;
    extern hsize_t initial_dataset_size;
}

#endif