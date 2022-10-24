make clean

make all

touch numbers.txt hashes.txt

./Generator 50

./Dispatcher hashes.txt > output.txt

diff output.txt numbers.txt

if [ $? -eq 0 ]; then
    echo "Test passed"
else
    echo "Test failed"
fi