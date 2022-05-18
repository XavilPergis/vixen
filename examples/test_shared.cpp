#include <vixen/allocator/allocators.hpp>
#include <vixen/shared.hpp>
#include <vixen/unique.hpp>
#include <vixen/vector.hpp>

using vixen::Vector;
using vixen::Weak;

void printWeakStatus(Weak<Vector<i32>> &weak) {
    if (auto inner = weak.upgrade()) {
        inner->insertLast(1917);
        VIXEN_INFO("upgrade successful! {}", *inner);
    } else {
        VIXEN_INFO("upgrade failed...");
    }
}

int main() {
    vixen::heap::ArenaAllocator tmp(vixen::heap::globalAllocator());

    Weak<Vector<i32>> weakRef;

    {
        VIXEN_INFO("making vector");
        Vector<i32> vec{tmp};
        vec.insertLast(5);
        vec.insertLast(4);
        vec.insertLast(3);
        vec.insertLast(2);
        vec.insertLast(1);

        VIXEN_INFO("moving vector into strong ref");
        vixen::Shared<Vector<i32>> strongRef{tmp, mv(vec)};

        VIXEN_INFO("downgrading strong ref");
        weakRef = strongRef.downgrade();
        strongRef->insertLast(10);
        strongRef->insertLast(9);
        strongRef->insertLast(8);
        strongRef->insertLast(7);
        strongRef->insertLast(6);
        strongRef->insertLast(1);
        strongRef->insertLast(1);
        strongRef->insertLast(1);

        VIXEN_INFO("copying strong ref to inner scope");
        {
            auto inner = strongRef.copy();
            inner->insertLast(12345678);
        }

        VIXEN_INFO("upgrading weak ref");
        printWeakStatus(weakRef);

        for (usize i = 0; i < strongRef->len(); ++i) {
            VIXEN_INFO("{}: {}", i, strongRef.get()[i]);
        }
    }

    VIXEN_INFO("creating second weak ref after strong ref destroyed");
    Weak<Vector<i32>> weakRefCopy = weakRef.copy();

    printWeakStatus(weakRef);
    printWeakStatus(weakRefCopy);

    return 0;
}