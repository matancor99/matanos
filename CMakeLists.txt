# This is a cmake for mattos.
# In my oppinion this cmake has 3 limitations:
# 1. in some parts it require abs paths - in the linker script and in the nasm %includes. Some places in the CMakelists.txt itself are with abs path also because of
# Problems of making it work, I guess this can be reduced.
# 2. When working with nasm on x86 when loading the cmakelists.txt it creates build.make file. It will contains nasm compilation lines with -f elf64 flag.
# I couldnt find any supported way to remove this flag so I simply created a dependand targed that removed this with sed.
# 3. working with ld directly is also very shitty - look at set(CMAKE_C_LINK_EXECUTABLE ...)

# ** TODO Before running - need to change the STARTUP in link.ld before running


cmake_minimum_required(VERSION 3.14)
project(matanos VERSION 1.0 LANGUAGES C ASM_NASM)
set(CMAKE_C_STANDARD 99)


#add_definitions( -DCMAKE_PROJECT_NAME=${CMAKE_PROJECT_NAME}  -DCMAKE_PROJECT_VERSION=${CMAKE_PROJECT_VERSION})


include_directories(boot cpu drivers kernel)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_C_COMPILER /usr/bin/gcc-7)
set(CMAKE_ASM_NASM_COMPILER /usr/bin/nasm)

# There was a bug in nasm - need to edit build.make and remove the "-f elf64" from the nasm lines.
set(CMAKE_C_FLAGS "-g -mgeneral-regs-only -mno-red-zone -fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-pie -ffreestanding -m32")
#set(CMAKE_EXE_LINKER_FLAGS "-T/root/CLionProjects/matanos/link.ld -m elf_i386 ")
#set(CMAKE_ASM_NASM_FLAGS "-f elf")
# This is shitty but the only way to do this
set(CMAKE_C_LINK_EXECUTABLE "/usr/bin/ld -T/root/CLionProjects/matanos/link.ld -m elf_i386 --oformat binary <OBJECTS>  -o <TARGET> ")


file(GLOB SOURCES
        "*.c"
        "boot/*.c"
        "cpu/*.c"
        "drivers/*.c"
        "kernel/*.c"
        "boot/bootsect.asm"
        "boot/kernel_entry.asm"
        "cpu/*.asm"
        )

message( STATUS "SOURCES ARE : ${SOURCES}")


add_executable(kernel.elf ${SOURCES})


# add the install targets
#install(TARGETS kernel.elf DESTINATION "${PROJECT_BINARY_DIR}/..")

# I had to add a stuiped patch for build.make - becasue I couldnt find a way to remove the -f elf64 when adding a nasm line
add_custom_target(command_target
        COMMAND sed -i 's/64//g' '/root/CLionProjects/matanos/cmake-build-debug/CMakeFiles/kernel.elf.dir/build.make'
        COMMENT "Running Costume Command"
        )

add_dependencies(kernel.elf command_target)

add_custom_command(TARGET kernel.elf
        POST_BUILD
        COMMAND qemu-system-i386 -fda ${PROJECT_BINARY_DIR}/kernel.elf
        COMMENT "Running Costume Command"
        )