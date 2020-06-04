#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "date.h"
#include "zmq.h"
#include "writer_config.hpp"
#include "buffer_config.hpp"
#include "bitshuffle/bitshuffle.h"
#include "JFH5Writer.hpp"
#include "ImageAssembler.hpp"
#include "BufferBinaryReader.hpp"

using namespace std;
using namespace writer_config;
using namespace buffer_config;
using namespace chrono;

void read_buffer(
        const string device,
        const string channel_name,
        const int i_module,
        const vector<uint64_t>& buffer_blocks,
        ImageAssembler& image_assembler)
{
    BufferBinaryReader block_reader(device, channel_name);
    auto block_buffer = new BufferBinaryBlock();

    for (uint64_t block_id:buffer_blocks) {

        while(!image_assembler.is_slot_free(block_id)) {
            this_thread::sleep_for(chrono::milliseconds(
                    WRITER_IMAGE_ASSEMBLER_RETRY_MS));
        }

        auto start_time = steady_clock::now();

        block_reader.get_block(block_id, block_buffer);

        auto end_time = steady_clock::now();
        uint64_t read_us_duration = duration_cast<microseconds>(
                end_time-start_time).count();

        start_time = steady_clock::now();

        image_assembler.process(block_id, i_module, block_buffer);

        end_time = steady_clock::now();
        uint64_t compose_us_duration = duration_cast<microseconds>(
                end_time-start_time).count();

        cout << "sf_writer:avg_read_us ";
        cout << read_us_duration / BUFFER_BLOCK_SIZE << endl;
        cout << "sf_writer:avg_assemble_us ";
        cout << compose_us_duration / BUFFER_BLOCK_SIZE << endl;
    }

    delete block_buffer;
}

int main (int argc, char *argv[])
{
    if (argc != 6) {
        cout << endl;
        cout << "Usage: sf_writer [output_file] [device]";
        cout << "  [n_modules]";
        cout << "  [start_pulse_id] [stop_pulse_id]";
        cout << endl;
        cout << "\toutput_file: Complete path to the output file." << endl;
        cout << "\tdevice: Name of detector." << endl;
        cout << "\tn_modules: number of modules" << endl;
        cout << "\tstart_pulse_id: Start pulse_id of retrieval." << endl;
        cout << "\tstop_pulse_id: Stop pulse_id of retrieval." << endl;
        cout << endl;

        exit(-1);
    }

    string output_file = string(argv[1]);
    const string device = string(argv[2]);
    size_t n_modules = atoi(argv[3]);
    //size_t n_modules = 32;
    uint64_t start_pulse_id = (uint64_t) atoll(argv[4]);
    uint64_t stop_pulse_id = (uint64_t) atoll(argv[5]);

    uint64_t start_block = start_pulse_id / BUFFER_BLOCK_SIZE;
    uint64_t stop_block = stop_pulse_id / BUFFER_BLOCK_SIZE;

    ImageAssembler image_assembler(n_modules);

    // Generate list of buffer blocks that need to be loaded.
    std::vector<uint64_t> buffer_blocks;
    for (uint64_t curr_block=start_block;
         curr_block<=stop_block;
         curr_block++) {
        buffer_blocks.push_back(curr_block);
    }

    std::vector<std::thread> reading_threads(n_modules);
    for (size_t i_module=0; i_module<n_modules; i_module++) {

        // TODO: Very ugly. Fix.
        string channel_name = "M";
        if (i_module < 10) {
            channel_name += "0";
        }
        channel_name += to_string(i_module);

        reading_threads.emplace_back(
                read_buffer,
                device,
                channel_name,
                i_module,
                ref(buffer_blocks),
                ref(image_assembler));
    }

    JFH5Writer writer(output_file, start_pulse_id, stop_pulse_id, n_modules);

    for (uint64_t block_id:buffer_blocks) {

        while(!image_assembler.is_slot_full(block_id)) {
            this_thread::sleep_for(chrono::milliseconds(
                    WRITER_IMAGE_ASSEMBLER_RETRY_MS));
        }

        auto metadata = image_assembler.get_metadata_buffer(block_id);
        auto data = image_assembler.get_data_buffer(block_id);

        auto start_time = steady_clock::now();

        writer.write(metadata, data);

        auto end_time = steady_clock::now();
        auto write_us_duration = chrono::duration_cast<chrono::microseconds>(
                end_time-start_time).count();

        image_assembler.free_slot(block_id);

        cout << "sf_writer:avg_write_us ";
        cout << write_us_duration / BUFFER_BLOCK_SIZE << endl;
    }

    for (auto& reading_thread :reading_threads) {
        if (reading_thread.joinable()) {
            reading_thread.join();
        }
    }

    return 0;
}
