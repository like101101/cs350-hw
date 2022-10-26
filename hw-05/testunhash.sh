make clean

make UnHash

make Generator

if [ -f numbers.txt ]; then
    rm numbers.txt
fi

if [ -f hashes.txt ]; then
    rm hashes.txt
fi

if [ ! -f Generator ]; then
    echo "generator not made"
    exit 1
fi

if [ ! -f UnHash ]; then
    echo "UnHash not made"
    exit 1
fi


./Generator 1 1

./UnHash $(cat hashes.txt) > output.txt

if [ ! -f output.txt ]; then
    echo "output.txt not made"
    exit 1
fi

if [[ $(wc -l output.txt) == 50 ]]; then
    echo "Incorrect amount of output"
    exit 1
fi

diff output.txt numbers.txt

if [ $? -eq 0 ]; then
    echo "Test passed"
else
    echo "Test failed"
fi

make clean