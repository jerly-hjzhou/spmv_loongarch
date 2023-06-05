NUM=10
ENABLE_PROFILE=1
OUT=output
mkdir -p $OUT

PROGRAM=$1
if [ -z "$PROGRAM" ]; then
    echo "Usage: $0 [program]"
    exit 1
fi

for ((INDEX = 0; INDEX <= NUM; INDEX++)); do
  INPUT_MATRIX=./accuracy/matrix$(printf "%02d" $INDEX).txt
  INPUT_VEC=./accuracy/vec$(printf "%02d" $INDEX).txt
  GOLDEN=./$OUT/golden$(printf "%02d" $INDEX).txt
  RESULT=./$OUT/result$(printf "%02d" $INDEX).txt
  

  echo "SPMV implemented by Eigen"
  PROFILING=$ENABLE_PROFILE ENABLE_DEBUG=0  ./eigen $INPUT_MATRIX $INPUT_VEC $GOLDEN
  echo "SPMV implemented by $PROGRAM"
  PROFILING=$ENABLE_PROFILE ENABLE_DEBUG=0  $PROGRAM $INPUT_MATRIX $INPUT_VEC $RESULT
  echo ""

  # 检查文件是否存在
  if [ ! -f "$GOLDEN" ]; then
    echo "文件 $GOLDEN 不存在"
    exit 1
  fi

  if [ ! -f "$RESULT" ]; then
    echo "文件 $RESULT 不存在"
    exit 1
  fi

  # 逐行比较两个文件
  line_number=1  # 初始化行数计数器
  while  IFS= read -r line2 <&3; do
    IFS= read -r line1
    diff=$(echo "scale=3; $line1 - $line2" | bc)
    diff_abs=$(echo "scale=3; if ($diff < 0) -($diff) else $diff" | bc)
    threshold=0.001
    #if (( $(echo "$diff_abs > $threshold" | bc -l) )); then
    if [ $line1 != $line2 ]; then
      echo "测试${INDEX}未通过，差异所在行数: $line_number"
      echo "${GOLDEN}: $line1"
      echo "${RESULT}: $line2"
      exit 1
    fi
    ((line_number++))  # 增加行数计数器
  done <"$GOLDEN" 3<"$RESULT"

done

echo "Algorithm performs correctly!"
