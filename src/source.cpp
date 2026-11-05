#include "source.hpp"

#include <mini.c/fs.h>
#include <mini.cc/fs.h>

Source_File::Source_File(String path, String canon_path, Alloc allocator)
    : allocator(allocator), path(path), canon_path(canon_path), line_starts(allocator)
{
    content = String(allocator);
    MINI_ASSERT(mini_fs_read_into(path.data(), &content.data()), "failed to read file at path: %s", path.data());

    line_starts.append(0);

    for (usize n = 0; n < content.count(); n++) {
        if (content[n] == '\n') {
            line_starts.append(n + 1);
        }
    }
}

Source_Manager::Source_Manager(Alloc alloc)
    : sources(alloc)
{}


Source_Manager::Source_Manager(Source_Manager&& other)
    : sources(std::move(other.sources))
{
}

Source_Manager& Source_Manager::operator=(Source_Manager&& other)
{
    if (this == &other) return *this;
    sources = std::move(other.sources);
    return *this;
}


Source_Id Source_Manager::open_source_file(
        MOVE String path,
        Alloc allocator)
{
    MINI_ASSERT(mini_fs_exists(path.data()), "path does not exist");
    auto canon_path = String(mini::fs::canonicalize(
            Sv(path.data()), allocator));

    // check if we already have this source file opened.
    if (sources.contains(canon_path)) {
        auto id = Source_Id(*sources.get_id(canon_path));
        return id;
    }

    auto source = Source_File(path, canon_path, allocator);
    return Source_Id(sources.insert(canon_path, source));
}

Sv Source_Manager::get_content_of(Source_Id source_id)
{
    auto *source = sources.at_index_ptr(usize(source_id));
    MINI_ASSERT(source != nullptr, "cannot retrieve source file at index: %zu\n", usize(source_id));
    return Sv(source->content.data());
}

const Source_File *Source_Manager::get_source(Source_Id source_id) const
{
    return sources.at_index_ptr(usize(source_id));
}

Source_File *Source_Manager::get_source(Source_Id source_id)
{
    return sources.at_index_ptr(usize(source_id));
}
