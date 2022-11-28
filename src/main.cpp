#include <iostream>
#include "Str.h"
#include "Arr.h"
#include "System.h"
#include "File.h"

// #define DEBUG

#ifdef DEBUG
const ml::Str EXAPLE_FOLDER_PATH = "/home/andrew/Documents/projects/cpp/BridgesFetcher/example";
const ml::Str TORRC_FILEPATH = EXAPLE_FOLDER_PATH + "/torrc";
const ml::Str BRIDGES_FILEPATH = EXAPLE_FOLDER_PATH + "/bridges";
const ml::Str USER_ID_FILEPATH = EXAPLE_FOLDER_PATH + "/user_id";
#else
const ml::Str TORRC_FILEPATH = "/etc/tor/torrc";
const ml::Str BRIDGES_FILEPATH = "/home/user/br_f/bridges";
const ml::Str USER_ID_FILEPATH = "/home/user/br_f/user_id";
#endif

const char* BRIDGES_LINK = "https://raw.githubusercontent.com/Artvenn/br/main/bridges";
const char* BLOCK_SPLITTER = "##############################";
const char* BRIDGES_START_MARK = "###bridges_start###";

enum Mode {SHUFFLE, FETCH_SHUFFLE};

i32 main(i32 argc, const char* argv[]) {
    Mode mode = Mode::FETCH_SHUFFLE;
    if (argc > 1) {
        mode = (ml::Str(argv[1]) == ml::Str("--shuffle")) 
            ? Mode::SHUFFLE 
            : Mode::FETCH_SHUFFLE;
    }

    ml::File user_id_file(USER_ID_FILEPATH);
    ml::Str user_id = user_id_file.read();

    if (mode == Mode::FETCH_SHUFFLE) {
        remove(BRIDGES_FILEPATH.to_c_str());
        ml::Sys::exec(ml::Str("wget -O ") + BRIDGES_FILEPATH + " " +  ml::Str(BRIDGES_LINK));
    } 

    ml::File bridges_file(BRIDGES_FILEPATH);
    ml::Str bridges_text = bridges_file.read();
    auto bridge_blocs = bridges_text.split(ml::Str(BLOCK_SPLITTER));
    i32 finded_user_index = bridge_blocs.first_where([user_id] (ml::Str el) -> bool {
        return el.starts_with(user_id);
    });
    if (finded_user_index == -1) {
        std::cerr << "User with id: " << user_id 
        << ", is not in bridges file" << std::endl;
        bridges_file.write("Bridge empty");
        exit(-1);
    }
    auto bridge_rows = bridge_blocs[finded_user_index].split('\n');
    bridge_rows.unshift();
    bridge_rows.shuffle();
    auto bridges = bridge_rows.join('\n');

    ml::File torrc_file(TORRC_FILEPATH);
    auto torrc_rows = torrc_file
        .read()
        .trim()
        .split('\n')
        .filter([] (ml::Str el) -> bool {
            return !el.starts_with("Bridge");
        });
    auto bridge_mark_index = torrc_rows.first_where([] (auto el) -> bool {
        return el == BRIDGES_START_MARK;
    });
    if (bridge_mark_index == -1) {
        std::cerr << "Fatal error:\t bridge start mark: " << BRIDGES_START_MARK 
        << "; is not found in file: " << TORRC_FILEPATH << std::endl;
        exit(-1);
    }
    torrc_rows.insert(bridge_mark_index+1, bridges);
    auto torrc_ready = torrc_rows.join('\n');
    torrc_file.write(torrc_ready);

    return 0;
}