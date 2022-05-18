// #pragma once

// namespace vixen::test {

// // generate, verify, shrink
// // struct aaa {};

// template <typename T>
// T generate(RandomSource &random);

// void test(RandomSource &random) {
//     auto vec = generate<Vector<i32>>(random);

//     vec.insertLast(123);
//     auto res = vec.removeLast();
//     VERIFY_EQ(res, 123);
// }

// } // namespace vixen::test