#include <vixen/allocator/allocators.hpp>
#include <vixen/shared.hpp>
#include <vixen/unique.hpp>
#include <vixen/vec.hpp>

void print_weak_status(vixen::Weak<vixen::Vector<i32>> &Weak) {
    if (auto inner = Weak.upgrade()) {
        (*inner)->push(1917);
        VIXEN_INFO("upgrade successful! {}", *inner);
    } else {
        VIXEN_INFO("upgrade failed...");
    }
}

int main() {
    vixen::heap::ArenaAllocator tmp(vixen::heap::global_allocator());

    vixen::Weak<vixen::Vector<i32>> my_weak;

    {
        VIXEN_INFO("making foo");
        vixen::Vector<i32> foo(&tmp);
        foo.push(5);
        foo.push(4);
        foo.push(3);
        foo.push(2);
        foo.push(1);

        VIXEN_INFO("making bar");
        vixen::shared<vixen::Vector<i32>> bar(&tmp, mv(foo));
        VIXEN_INFO("making Weak");
        my_weak = bar.downgrade();
        bar->push(10);
        bar->push(9);
        bar->push(8);
        bar->push(7);
        bar->push(6);
        bar->push(1);
        bar->push(1);
        bar->push(1);

        VIXEN_INFO("copying bar");
        {
            auto inner = bar.copy();
            inner->push(12345678);
        }

        VIXEN_INFO("upgrading Weak inner");
        print_weak_status(my_weak);

        VIXEN_INFO("bar = {}", bar);

        for (usize i = 0; i < bar->len(); ++i) {
            VIXEN_INFO("{}: {}", i, (*bar)[i]);
        }
    }

    VIXEN_INFO("creating second Weak");
    vixen::Weak<vixen::Vector<i32>> my_weak_2 = my_weak.copy();

    print_weak_status(my_weak);
    print_weak_status(my_weak_2);

    return 0;
}