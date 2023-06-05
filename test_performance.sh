DATASET=("ASIC_680k.mtx" "FEM_3D_thermal2.mtx" "af_shell1.mtx" "amazon0601.mtx"
         "as-Skitter.mtx" "boneS10.mtx" "com-Youtube.mtx" "delaunay_n19.mtx"
         "hugetric-00020.mtx" "in-2004.mtx" "ldoor.mtx" "mc2depi.mtx"
         "memchip.mtx" "parabolic_fem.mtx" "pkustk14.mtx" "poisson3Db.mtx"
         "rajat31.mtx" "roadNet-TX.mtx" "sx-stackoverflow.mtx" "thermomech_dK.mtx"
         "web-Google.mtx" "webbase-1M.mtx")
ITERATION=1

OUT=output
mkdir -p $OUT
RESULT=./$OUT/result.txt

PROGRAM=$1
if [ -z "$PROGRAM" ]; then
    echo "Usage: $0 [program]"
    exit 1
fi

for NAME in "${DATASET[@]}"
do
    INPUT_MATRIX="./dataset/$NAME"
    INPUT_VEC="./dataset/vec_$NAME"
    if [ ! -f "$INPUT_MATRIX" ]; then
        echo "文件 $INPUT_MATRIX 不存在"
        exit 1
    fi
    if [ ! -f "$INPUT_VEC" ]; then
        echo "文件 $INPUT_VEC 不存在"
        exit 1
    fi

    SUM=0
    echo "[DATA=$NAME] [iteration=$ITERATION]"
    for ((INDEX = 0; INDEX < ITERATION; INDEX++)); do
        output=$(PROFILING=1 ./spmv "$INPUT_MATRIX" "$INPUT_VEC" "$RESULT")
        op_time=$(echo "$output" | grep 'Execution time of \[OP\]' | grep -oE '[0-9]+' | awk '{print $1}')
        SUM=$((SUM + op_time))
    done
    NAIVE=$(bc <<< "scale=3; $SUM / $ITERATION")
    echo "1. [Naive] Average Kernel Time: $(echo $NAIVE)us"

    SUM=0
    for ((INDEX = 0; INDEX < ITERATION; INDEX++)); do
        output=$(PROFILING=1 $PROGRAM "$INPUT_MATRIX" "$INPUT_VEC" "$RESULT")
        op_time=$(echo "$output" | grep 'Execution time of \[OP\]' | grep -oE '[0-9]+' | awk '{print $1}')
        SUM=$((SUM + op_time))
    done
    MY_TIME=$(bc <<< "scale=3; $SUM / $ITERATION")
    echo "2. [$PROGRAM] Average Kernel Time: $(echo $MY_TIME)us"
    echo "3. Speedup : $(echo "scale=3; $NAIVE / $MY_TIME" | bc -l)"

done
