#include <vixen/allocator/allocators.hpp>
#include <vixen/unique.hpp>
#include <vixen/vec.hpp>

int main() {
    vixen::heap::arena_allocator tmp(vixen::heap::global_allocator());

    vixen::vector<i32> foo(&tmp);
    foo.push(5);
    foo.push(4);
    foo.push(3);
    foo.push(2);
    foo.push(1);

    vixen::unique<vixen::vector<i32>> bar(&tmp, MOVE(foo));
    bar->push(10);
    bar->push(9);
    bar->push(8);
    bar->push(7);
    bar->push(6);
    bar->push(1);
    bar->push(1);
    bar->push(1);

    VIXEN_INFO("bar = {}", bar);

    for (usize i = 0; i < bar->len(); ++i) {
        VIXEN_INFO("{}: {}", i, (*bar)[i]);
    }

    return 0;
}