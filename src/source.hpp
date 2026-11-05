#pragma once

#include "misc/misc.hpp"
#include "misc/densemap.hpp"

enum struct Source_Id : usize {};

struct Source_File {
    Mini_Allocator allocator;

    String path, canon_path;
    String content;

    mini::Array<usize> line_starts;

    Source_File(MOVE String path, MOVE String canon_path, Alloc allocator);
};

struct Source_Manager {
    DenseMap<String, Source_File> sources;

    Source_Manager(Alloc alloc);

    Source_Manager(const Source_Manager&) = delete;
    Source_Manager& operator=(const Source_Manager&) = delete;

    Source_Manager(Source_Manager&&);
    Source_Manager& operator=(Source_Manager&&);

    Source_Id open_source_file(MOVE String path, Alloc allocator);

    Opt<Source_Id> find_id_of(Sv path);

    const Source_File *get_source(Source_Id source_id) const;
    Source_File *get_source(Source_Id source_id);

    Sv get_content_of(Source_Id source_id);
};
