#!/usr/bin/env bash

echo -e "\e[96mLinter Test\e[0m"
# Use Linux kernel style
config=test/ament_code_style.cfg
(( errors=0 ))

crusty=$(which uncrustify)
# make sure we have uncrustify
if [[ -z $crusty ]]; then
    echo -e "\e[31mPlease install uncrustify!\e[0m"
    echo -e "\e[31m    sudo apt-get install uncrustify\e[0m"
    exit 1
fi

# make sure we are in the right place
if [[ ! -f LICENSE ]]; then
    echo -e "\e[31mRun linter script from the root of the repo.\e[0m"
    exit 2
fi

# Lint the sources.
for cpp_file in src/*.cpp; do
    uncrustify -c $config $cpp_file
    dif=$(diff -ur $cpp_file $cpp_file.uncrustify)

    if [[ ! -z $dif ]]; then
        echo -e "\[31m$cpp_file does not conform to ROS 2 style!"
        echo "Patch will be found in $cpp_file.patch."
        diff -ur $cpp_file $cpp_file.uncrustify > $cpp_file.patch
        rm $cpp_file.uncrustify
        echo "You may apply the patch like this:"
        echo -e "    patch -p1 $cpp_file < $cpp_file.patch\e[0m"
        exit 3
    fi
    rm $cpp_file.uncrustify
done

# version check
# TODO(allenh1): when travis leaves trusty, switch to this.
# temp_file=$(mktemp)
# uncrustify --version > ${temp_file}
# sed -i -e 's/Uncrustify-//g' ${temp_file}
# sed -i -e 's/_[a-zA-Z]//g' ${temp_file}
# version=$(cat ${temp_file})
# rm -f ${temp_file}
ver_string=$(uncrustify --version | awk '{print $1}')

if [[ $ver_string = *'uncrustify'* ]]; then
    echo -e "\e[31mUncrustify cannot test headers!\e[0m"
    echo -e "\e[32mPretending things are fine...\e[0m"
    exit 0
fi

# Lint the headers
for c_file in src/*.hpp; do
    uncrustify -c $config $c_file
    dif=$(diff -ur $c_file $c_file.uncrustify)

    if [[ ! -z $dif ]]; then
        echo -e "\e[31m$c_file does not conform to ROS 2 style!"
        echo "Patch will be found in $c_file.patch."
        diff -ur $c_file $c_file.uncrustify > $c_file.patch
        rm $c_file.uncrustify
        echo "You may apply the patch like this:"
        echo -e "    patch -p1 $c_file < $c_file.patch\e[0m"
        exit 3
    fi
    rm $c_file.uncrustify
done

echo -e "\e[32mLinter test passed\e[0m"
exit 0
