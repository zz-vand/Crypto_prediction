
PARSER_INPUT_DIR="/app/data/parser_input"
MODEL_OUTPUT_DIR="/app/data/model_output"
RESULTS_DIR="/app/data/results"
LOG_FILE="/app/logs/pipeline_$(date +%Y%m%d_%H%M%S).log"

mkdir -p "$PARSER_INPUT_DIR" "$MODEL_OUTPUT_DIR" "$RESULTS_DIR" "$(dirname "$LOG_FILE")"

exec > >(tee -a "$LOG_FILE") 2>&1

echo "=== Crypto Predictor Pipeline ==="
echo "Start time: $(date)"
echo "--------------------------------"

check_status() {
    if [ $? -ne 0 ]; then
        echo "[ERROR] $1 failed at $(date)" | tee -a "$LOG_FILE"
        exit 1
    fi
}

echo "[1/3] Running Data Parser..."
echo "Processing top 10 crypto..."
/app/bin/parser
check_status "Data Parser"

PARSER_FILES=("$PARSER_INPUT_DIR"/*_data_.json)
if [ ${#PARSER_FILES[@]} -eq 0 ]; then
    echo "[ERROR] No files found in $PARSER_INPUT_DIR" | tee -a "$LOG_FILE"
    ls -la "$PARSER_INPUT_DIR" | tee -a "$LOG_FILE"
    exit 1
fi
echo "Generated ${#PARSER_FILES[@]} data files"

echo "[2/3] Running Prediction Models..."
processed_symbols=0

for input_file in "${PARSER_FILES[@]}"; do
    symbol=$(basename "$input_file" | cut -d'_' -f1)
    output_file="$MODEL_OUTPUT_DIR/predictions_$(basename "$input_file")"
    
    if [ -f "$output_file" ] && [ "$output_file" -nt "$input_file" ]; then
        echo "Skipping $symbol - prediction already exists and is up-to-date"
        continue
    fi
    
    echo "Processing $symbol..."
    start_time=$(date +%s)
    
    python3 /app/src/python/model/model.py "$input_file"
    check_status "Model for $symbol"
    
    end_time=$(date +%s)
    echo "Completed $symbol in $((end_time - start_time)) seconds"
    ((processed_symbols++))
done

MODEL_FILES=("$MODEL_OUTPUT_DIR"/predictions_*.json)
if [ ${#MODEL_FILES[@]} -eq 0 ]; then
    echo "[ERROR] No model output files found in $MODEL_OUTPUT_DIR" | tee -a "$LOG_FILE"
    ls -la "$MODEL_OUTPUT_DIR" | tee -a "$LOG_FILE"
    exit 1
fi
echo "Processed $processed_symbols new predictions (total ${#MODEL_FILES[@]} prediction files)"

echo "[3/3] Creating Visualizations..."
start_time=$(date +%s)

/app/bin/visualization
check_status "Visualization"

VIS_FILES=("$RESULTS_DIR"/*.png)
if [ ${#VIS_FILES[@]} -eq 0 ]; then
    echo "[ERROR] No visualization files found in $RESULTS_DIR" | tee -a "$LOG_FILE"
    ls -la "$RESULTS_DIR" | tee -a "$LOG_FILE"
    exit 1
fi

end_time=$(date +%s)
echo "Generated ${#VIS_FILES[@]} visualization files in $((end_time - start_time)) seconds"
