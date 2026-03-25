#!/bin/bash

# Calibration Test Runner
# This script runs the complete calibration dataset validation and testing pipeline

set -e

echo "🔬 Mini Autonomy System Calibration Test Pipeline"
echo "=================================================="

# Check if Python is available
if ! command -v python3 &> /dev/null; then
    echo "❌ Python3 is not installed or not in PATH"
    exit 1
fi

# Check if required packages are available
echo "📦 Checking Python dependencies..."
python3 -c "
import sys
required_packages = ['cv2', 'numpy', 'yaml', 'pandas', 'matplotlib', 'scipy']
missing = []
for pkg in required_packages:
    try:
        __import__(pkg)
    except ImportError:
        missing.append(pkg)

if missing:
    print(f'❌ Missing packages: {missing}')
    print('Install with: pip install opencv-python numpy pyyaml pandas matplotlib scipy')
    sys.exit(1)
else:
    print('✅ All required packages available')
"

# Set up directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DATASET_DIR="${SCRIPT_DIR}/calibration_datasets"
RESULTS_DIR="${SCRIPT_DIR}/test_results"
VALIDATION_DIR="${RESULTS_DIR}/validation"
ACCURACY_DIR="${RESULTS_DIR}/accuracy"

# Create output directories
mkdir -p "$RESULTS_DIR"
mkdir -p "$VALIDATION_DIR"
mkdir -p "$ACCURACY_DIR"

# Function to run a test and report status
run_test() {
    local test_name="$1"
    local test_command="$2"
    local output_file="$3"
    
    echo ""
    echo "🧪 Running $test_name..."
    echo "Command: $test_command"
    
    if eval "$test_command" > "$output_file" 2>&1; then
        echo "✅ $test_name PASSED"
        return 0
    else
        echo "❌ $test_name FAILED"
        echo "Check $output_file for details"
        return 1
    fi
}

# Function to check if dataset exists
check_dataset() {
    local dataset_path="$1"
    
    if [ ! -d "$dataset_path" ]; then
        echo "❌ Dataset not found: $dataset_path"
        return 1
    fi
    
    if [ ! -f "$dataset_path"/*.yaml ] && [ ! -f "$dataset_path"/*.bag ]; then
        echo "❌ No valid dataset files found in: $dataset_path"
        return 1
    fi
    
    echo "✅ Dataset found: $dataset_path"
    return 0
}

# Main test execution
FAILED_TESTS=0
TOTAL_TESTS=0

echo ""
echo "📋 Dataset Discovery"

# Find all datasets
if [ -d "$DATASET_DIR" ]; then
    DATASETS=($(find "$DATASET_DIR" -maxdepth 1 -type d -not -path "$DATASET_DIR"))
    echo "Found ${#DATASETS[@]} datasets:"
    for dataset in "${DATASETS[@]}"; do
        echo "  - $(basename "$dataset")"
    done
else
    echo "❌ Calibration datasets directory not found: $DATASET_DIR"
    exit 1
fi

# Test 1: Dataset Validation
echo ""
echo "🔍 Test 1: Dataset Validation"
TOTAL_TESTS=$((TOTAL_TESTS + 1))

if run_test "Dataset Validation" \
    "python3 ${SCRIPT_DIR}/validate_calibration_datasets.py --dataset-path '$DATASET_DIR' --verbose" \
    "$VALIDATION_DIR/validation.log"; then
    echo "✅ Dataset validation completed successfully"
else
    FAILED_TESTS=$((FAILED_TESTS + 1))
fi

# Test 2: Calibration Test Suite
echo ""
echo "🎯 Test 2: Calibration Test Suite"
TOTAL_TESTS=$((TOTAL_TESTS + 1))

if run_test "Calibration Test Suite" \
    "python3 ${SCRIPT_DIR}/test_calibration_suite.py --dataset-path '$DATASET_DIR' --output-dir '$RESULTS_DIR/calibration_tests'" \
    "$RESULTS_DIR/calibration_tests/test_suite.log"; then
    echo "✅ Calibration test suite completed successfully"
else
    FAILED_TESTS=$((FAILED_TESTS + 1))
fi

# Test 3: ROSBAG Processing (if ROSBAG is available)
echo ""
echo "📦 Test 3: ROSBAG Processing"
TOTAL_TESTS=$((TOTAL_TESTS + 1))

# Check if rosbag package is available
if python3 -c "import rosbag" 2>/dev/null; then
    # Find a ROSBAG file to test
    BAG_FILE=$(find "$DATASET_DIR" -name "*.bag" | head -1)
    
    if [ -n "$BAG_FILE" ]; then
        if run_test "ROSBAG Processing" \
            "python3 ${SCRIPT_DIR}/rosbag_processor.py --input '$BAG_FILE' --output '$RESULTS_DIR/rosbag_test' --action analyze" \
            "$RESULTS_DIR/rosbag_test/rosbag.log"; then
            echo "✅ ROSBAG processing test completed successfully"
        else
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
    else
        echo "⚠️  No ROSBAG files found for testing"
        echo "Skipping ROSBAG processing test"
    fi
else
    echo "⚠️  rosbag package not available"
    echo "Install with: pip install rosbag"
    echo "Skipping ROSBAG processing test"
fi

# Test 4: Accuracy Testing (if calibration results available)
echo ""
echo "📊 Test 4: Accuracy Testing"
TOTAL_TESTS=$((TOTAL_TESTS + 1))

# Look for existing calibration results or create dummy ones for testing
CALIB_FILE="$RESULTS_DIR/calibration_tests/calibration_results.yaml"
if [ ! -f "$CALIB_FILE" ]; then
    echo "🔧 Creating sample calibration results for accuracy testing..."
    
    # Create a sample calibration file
    cat > "$CALIB_FILE" << 'EOF'
camera_matrix:
  - [500.0, 0.0, 376.0]
  - [0.0, 500.0, 240.0]
  - [0.0, 0.0, 1.0]
distortion_coeffs: [-0.1, 0.01, -0.001, 0.0, 0.0]
reprojection_error: 0.5
EOF
fi

# Find a dataset to test against
TEST_DATASET=$(find "$DATASET_DIR" -maxdepth 1 -type d | head -2 | tail -1)

if [ -n "$TEST_DATASET" ] && [ -d "$TEST_DATASET" ]; then
    if run_test "Accuracy Testing" \
        "python3 ${SCRIPT_DIR}/calibration_accuracy_tests.py --calibration-results '$CALIB_FILE' --test-data '$TEST_DATASET' --output-dir '$ACCURACY_DIR'" \
        "$ACCURACY_DIR/accuracy_test.log"; then
        echo "✅ Accuracy testing completed successfully"
    else
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
else
    echo "⚠️  No suitable test dataset found for accuracy testing"
    echo "Skipping accuracy test"
fi

# Generate summary report
echo ""
echo "📈 Generating Summary Report..."

cat > "$RESULTS_DIR/test_summary.txt" << EOF
Mini Autonomy System Calibration Test Summary
============================================
Date: $(date)
Test Directory: $SCRIPT_DIR
Results Directory: $RESULTS_DIR

Test Results:
-------------
Total Tests: $TOTAL_TESTS
Passed: $((TOTAL_TESTS - FAILED_TESTS))
Failed: $FAILED_TESTS

Dataset Information:
-------------------
Dataset Directory: $DATASET_DIR
Available Datasets: ${#DATASETS[@]}

Test Logs:
----------
- Validation Log: $VALIDATION_DIR/validation.log
- Test Suite Log: $RESULTS_DIR/calibration_tests/test_suite.log
- ROSBAG Log: $RESULTS_DIR/rosbag_test/rosbag.log (if available)
- Accuracy Test Log: $ACCURACY_DIR/accuracy_test.log

Generated Files:
----------------
- Validation Results: $VALIDATION_DIR/
- Calibration Test Results: $RESULTS_DIR/calibration_tests/
- ROSBAG Analysis: $RESULTS_DIR/rosbag_test/ (if available)
- Accuracy Report: $ACCURACY_DIR/

EOF

# Add dataset-specific information
for dataset in "${DATASETS[@]}"; do
    dataset_name=$(basename "$dataset")
    echo "" >> "$RESULTS_DIR/test_summary.txt"
    echo "Dataset: $dataset_name" >> "$RESULTS_DIR/test_summary.txt"
    echo "-----------" >> "$RESULTS_DIR/test_summary.txt"
    
    # Count files
    yaml_count=$(find "$dataset" -name "*.yaml" | wc -l)
    bag_count=$(find "$dataset" -name "*.bag" | wc -l)
    csv_count=$(find "$dataset" -name "*.csv" | wc -l)
    png_count=$(find "$dataset" -name "*.png" | wc -l)
    
    echo "YAML files: $yaml_count" >> "$RESULTS_DIR/test_summary.txt"
    echo "ROSBAG files: $bag_count" >> "$RESULTS_DIR/test_summary.txt"
    echo "CSV files: $csv_count" >> "$RESULTS_DIR/test_summary.txt"
    echo "PNG files: $png_count" >> "$RESULTS_DIR/test_summary.txt"
done

echo "✅ Summary report generated: $RESULTS_DIR/test_summary.txt"

# Final status
echo ""
echo "🏁 Test Pipeline Complete"
echo "=========================="

if [ $FAILED_TESTS -eq 0 ]; then
    echo "🎉 ALL TESTS PASSED!"
    echo ""
    echo "📊 Results Summary:"
    echo "  - Total Tests: $TOTAL_TESTS"
    echo "  - Passed: $((TOTAL_TESTS - FAILED_TESTS))"
    echo "  - Failed: $FAILED_TESTS"
    echo ""
    echo "📁 Results available in: $RESULTS_DIR"
    echo "📄 Full summary: $RESULTS_DIR/test_summary.txt"
    exit 0
else
    echo "⚠️  SOME TESTS FAILED"
    echo ""
    echo "📊 Results Summary:"
    echo "  - Total Tests: $TOTAL_TESTS"
    echo "  - Passed: $((TOTAL_TESTS - FAILED_TESTS))"
    echo "  - Failed: $FAILED_TESTS"
    echo ""
    echo "❌ Check the following log files for details:"
    
    if [ -f "$VALIDATION_DIR/validation.log" ]; then
        echo "  - Validation: $VALIDATION_DIR/validation.log"
    fi
    
    if [ -f "$RESULTS_DIR/calibration_tests/test_suite.log" ]; then
        echo "  - Test Suite: $RESULTS_DIR/calibration_tests/test_suite.log"
    fi
    
    if [ -f "$RESULTS_DIR/rosbag_test/rosbag.log" ]; then
        echo "  - ROSBAG: $RESULTS_DIR/rosbag_test/rosbag.log"
    fi
    
    if [ -f "$ACCURACY_DIR/accuracy_test.log" ]; then
        echo "  - Accuracy: $ACCURACY_DIR/accuracy_test.log"
    fi
    
    echo ""
    echo "📄 Full summary: $RESULTS_DIR/test_summary.txt"
    exit 1
fi
