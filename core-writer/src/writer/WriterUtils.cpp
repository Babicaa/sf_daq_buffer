#include <iostream>
#include <sys/fsuid.h>
#include "WriterUtils.hpp"
#include "date.h"

using namespace std;

void WriterUtils::set_fs_id(int user_id)
{

    #ifdef DEBUG_OUTPUT
        using namespace date;
        using namespace chrono;
        cout << "[" << system_clock::now() << "]";
        cout << "[WriterUtils::set_process_effective_id]";
        cout << " Setting process user to " << user_id << endl;
    #endif

    if (setfsgid(user_id)) {
        stringstream err_msg;

        using namespace date;
        using namespace chrono;
        err_msg << "[" << system_clock::now() << "]";
        err_msg << "[WriterUtils::set_fs_uid]";
        err_msg << " Cannot set group_id to " << user_id << endl;

        throw runtime_error(err_msg.str());
    }

    if (setfsuid(user_id)) {
        stringstream err_msg;

        using namespace date;
        using namespace chrono;
        err_msg << "[" << system_clock::now() << "]";
        err_msg << "[WriterUtils::set_fs_uid]";
        err_msg << " Cannot set user_id to " << user_id << endl;

        throw runtime_error(err_msg.str());
    }
}

void WriterUtils::create_destination_folder(const string& output_file)
{
    auto file_separator_index = output_file.rfind('/');

    if (file_separator_index != string::npos) {
        string output_folder(output_file.substr(0, file_separator_index));

        using namespace date;
        using namespace chrono;
        cout << "[" << system_clock::now() << "]";
        cout << "[WriterUtils::create_destination_folder]";
        cout << " Creating folder " << output_folder << endl;

        string create_folder_command("mkdir -p " + output_folder);
        system(create_folder_command.c_str());
    }
}
