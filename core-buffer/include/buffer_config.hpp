#ifndef BUFFERCONFIG_HPP
#define BUFFERCONFIG_HPP

#include <cstddef>
#include <string>

namespace core_buffer {

    const size_t MODULE_X_SIZE = 1024;
    const size_t MODULE_Y_SIZE = 512;
    const size_t MODULE_N_PIXELS = MODULE_X_SIZE * MODULE_Y_SIZE;
    const size_t MODULE_N_BYTES = MODULE_N_PIXELS * 2;

    // How many frames we store in each file.
    // Must be power of 10 and <= than FOLDER_MOD
    const size_t FILE_MOD = 1000;

    // How many frames go into each files folder.
    // Must be power of 10 and >= than FILE_MOD.
    const size_t FOLDER_MOD = 100000;

    // Extension of our file format.
    const std::string FILE_EXTENSION = ".h5";

    // How many frames do we read at once during replay.
    const size_t REPLAY_BLOCK_SIZE = 100;

    // Size of sf_buffer RB in elements.
    const size_t BUFFER_RB_SIZE = 1000;

    // ZMQ threads for receiving data from sf_replay.
    const int WRITER_ZMQ_IO_THREADS = 4;

    // Size of buffer between the receiving and writing part of sf_writer
    const int WRITER_RB_BUFFER_SLOTS = 10;

    // How many frames to buffer before flushing to file.
    const size_t WRITER_BUFFER_SIZE = 100;

    // Number of pulses between each statistics print out.
    const size_t STATS_MODULO = 100;
}

#endif //BUFFERCONFIG_HPP
