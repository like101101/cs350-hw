make clean

make all

./Generator 1

./UnHash $(cat hashes.txt) > output.txt

diff output.txt numbers.txt

if [ $? -eq 0 ]; then
    echo "Test passed"
else
    echo "Test failed"
fi