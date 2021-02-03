# Vixen

Vixen is a utility library, and a product of Not Implemented Here™. It attempts to fill the niche that C++'s standard library does, and libraries like boost do.
I'm a C++ newbie, so don't expect the code quality to be very high. :p

## Using
Vixen is meant to be used as a git submodule with CMake.

To add vixen to your repository, run the following:
```sh
$ git submodule add $MODULE_LOCATION https://github.com/XavilPergis/vixen.git
$ git submodule update --init --recursive
```

To add vixen as dependency to your CMake project:
```cmake
# Add the root directory of your Vixen submodule 
add_subdirectory(${MODULE_LOCATION})

# Link all of Vixen into your targets
target_link_libraries(${MY_PROJECT} PRIVATE vixen)
```

## Developing

### Project Structure
Vixen is organized into three main directories. The first is `vixen/interface`, where Vixen's public interface is located. Then, there's `vixen/impl`, which contains inline implementations in `vixen/impl/inl`, and out-of-line implementations in `vixen/impl/obj` (cpp files, which produce _obj_ect files)


