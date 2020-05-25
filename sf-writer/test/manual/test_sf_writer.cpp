#include <iostream>
#include <stdexcept>
#include "buffer_config.hpp"
#include "zmq.h"
#include <string>
#include <jungfrau.hpp>
#include <thread>
#include <chrono>
#include "WriterH5Writer.hpp"
#include <FastQueue.hpp>
#include <cstring>
#include <BufferedFastQueue.hpp>
#include "date.h"
#include "bitshuffle/bitshuffle.h"
#include "WriterZmqReceiver.hpp"

using namespace std;
using namespace core_buffer;

void receive_replay(
        void* ctx,
        const string ipc_prefix,
        const size_t n_modules,
        FastQueue<ImageMetadata>& queue,
        const uint64_t start_pulse_id,
        const uint64_t stop_pulse_id)
{
    try {
        WriterZmqReceiver receiver(ctx, ipc_prefix, n_modules);

        uint64_t current_pulse_id=start_pulse_id;

        // "<= stop_pulse_id" because we include the last pulse_id.
        while(current_pulse_id<=stop_pulse_id) {

            int slot_id;
            while((slot_id = queue.reserve()) == -1) {
                this_thread::sleep_for(chrono::milliseconds(
                        RB_READ_RETRY_INTERVAL_MS));
            }

            auto image_metadata = queue.get_metadata_buffer(slot_id);
            auto image_buffer = queue.get_data_buffer(slot_id);

            cout << "Received " << image_metadata->pulse_id << endl;

            receiver.get_next_batch(
                    current_pulse_id, image_metadata, image_buffer);

            if (image_metadata->pulse_id != current_pulse_id) {
                throw runtime_error("Wrong pulse id from zmq receiver.");
            }

            queue.commit();
            current_pulse_id++;
        }

    } catch (const std::exception& e) {
        using namespace date;
        using namespace chrono;

        cout << "[" << system_clock::now() << "]";
        cout << "[sf_writer::receive_replay]";
        cout << " Stopped because of exception: " << endl;
        cout << e.what() << endl;

        throw;
    }
}

int main (int argc, char *argv[])
{
    if (argc != 4) {
        cout << endl;
        cout << "Usage: sf_writer ";
        cout << " [output_file] [start_pulse_id] [stop_pulse_id]";
        cout << endl;
        cout << "\toutput_file: Complete path to the output file." << endl;
        cout << "\tstart_pulse_id: Start pulse_id of retrieval." << endl;
        cout << "\tstop_pulse_id: Stop pulse_id of retrieval." << endl;
        cout << endl;

        exit(-1);
    }

    string output_file = string(argv[1]);
    uint64_t start_pulse_id = (uint64_t) atoll(argv[2]);
    uint64_t stop_pulse_id = (uint64_t) atoll(argv[3]);

    size_t n_modules = 32;

    FastQueue<ImageMetadata> queue(
            MODULE_N_BYTES * n_modules,
            WRITER_FASTQUEUE_N_SLOTS);

    auto ctx = zmq_ctx_new();
    zmq_ctx_set (ctx, ZMQ_IO_THREADS, WRITER_ZMQ_IO_THREADS);

    thread replay_receive_thread(receive_replay,
                                 ctx, REPLAY_STREAM_IPC_URL, n_modules,
                                 ref(queue), start_pulse_id, stop_pulse_id);

    auto current_pulse_id = start_pulse_id;
    // "<= stop_pulse_id" because we include the last pulse_id.
    while (current_pulse_id <= stop_pulse_id) {

        int slot_id;
        while((slot_id = queue.read()) == -1) {
            this_thread::sleep_for(chrono::milliseconds(
                    RB_READ_RETRY_INTERVAL_MS));
        }

        auto metadata = queue.get_metadata_buffer(slot_id);

        cout << "Written image " << metadata->pulse_id << endl;
        current_pulse_id++;

        queue.release();
    }

    //wait till receive thread is finished
    replay_receive_thread.join();
    return 0;
}