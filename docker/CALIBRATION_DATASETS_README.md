# Calibration Datasets Documentation

This directory contains calibration datasets and comprehensive tools for validation, testing, and processing of calibration data for the Mini Autonomy System.

## Directory Structure

```
calibration_datasets/
├── cam_april/                    # AprilTag calibration dataset
│   ├── april_6x6.yaml          # Target configuration
│   ├── cam_april.bag           # ROSBAG V2.0 data
│   └── cam_april/              # Extracted data structure
│       └── mav0/               # Multi-sensor data
│           ├── cam0/           # Camera 0 data
│           │   ├── sensor.yaml # Camera configuration
│           │   ├── data.csv    # Timestamp and metadata
│           │   └── data/       # Extracted image frames
│           ├── cam1/           # Camera 1 data (if available)
│           └── imu0/           # IMU data
├── cam_checkerboard/           # Checkerboard calibration dataset
│   ├── checkerboard_7x6.yaml  # Target configuration
│   ├── cam_checkerboard.bag   # ROSBAG V2.0 data
│   └── cam_checkerboard/      # Extracted data structure
└── imu_april/                  # IMU + AprilTag dataset
    ├── april_6x6.yaml        # Target configuration
    ├── imu_april.bag          # ROSBAG V2.0 data
    └── imu_april/             # Extracted data structure
```

## Dataset Formats

### Configuration Files (.yaml)

#### AprilTag Configuration
```yaml
target_type: 'aprilgrid'
tagCols: 6                    # Number of AprilTags horizontally
tagRows: 6                    # Number of AprilTags vertically  
tagSize: 0.088               # Tag size in meters (edge to edge)
tagSpacing: 0.3              # Ratio of spacing between tags to tag size
```

#### Checkerboard Configuration
```yaml
target_type: 'checkerboard'
targetCols: 6                 # Number of internal chessboard corners
targetRows: 7                 # Number of internal chessboard corners
rowSpacingMeters: 0.06       # Size of one chessboard square in meters
colSpacingMeters: 0.06       # Size of one chessboard square in meters
```

#### Sensor Configuration
```yaml
sensor_type: 'camera'        # or 'imu'
comment: 'VI-Sensor cam0 (MT9M034)'
T_BS:                        # Transformation from body to sensor
  cols: 4
  rows: 4
  data: [0.0148655429818, -0.999880929698, ...]  # 4x4 matrix
rate_hz: 20                  # Sensor rate in Hz
resolution: [752, 480]       # Image resolution [width, height]
```

### Data Files

#### ROSBAG Files (.bag)
- Format: ROSBAG V2.0
- Contains raw sensor data streams
- Topics include:
  - `/cam0/image_raw` - Camera images
  - `/cam1/image_raw` - Secondary camera (if available)
  - `/imu0/data` - IMU measurements

#### CSV Data Files
- `data.csv` - Sensor data with timestamps
- Format: `timestamp,seq,frame_id,data`
- Used for synchronization and analysis

#### Extracted Frames
- Located in `data/` subdirectories
- Format: PNG images
- Naming: `{timestamp_ns}.png`
- Resolution: 752x480 pixels (typical)

## Available Tools

### 1. Dataset Validation
```bash
python validate_calibration_datasets.py --dataset-path calibration_datasets --verbose
```

**Features:**
- Validates dataset structure and file formats
- Checks YAML configuration syntax
- Verifies ROSBAG file integrity
- Validates CSV data format
- Reports missing or corrupted files

### 2. Calibration Test Suite
```bash
python test_calibration_suite.py --dataset-path calibration_datasets --output-dir test_results
```

**Features:**
- Target detection testing
- Calibration parameter estimation
- Reprojection accuracy evaluation
- Performance benchmarking
- Visual result generation

### 3. ROSBAG Processing
```bash
python rosbag_processor.py --input dataset.bag --output extracted_data --action generate_dataset
```

**Actions:**
- `analyze` - Analyze ROSBAG structure
- `extract_images` - Extract image frames
- `extract_sensor` - Extract sensor data to CSV
- `extract_imu` - Extract IMU data with specific formatting
- `generate_dataset` - Create complete calibration dataset

### 4. Accuracy Testing
```bash
python calibration_accuracy_tests.py --calibration-results calib.yaml --test-data calibration_datasets/cam_april
```

**Features:**
- Reprojection error analysis
- Cross-validation testing
- Calibration stability assessment
- Ground truth comparison
- Statistical analysis

## Usage Examples

### Basic Dataset Validation
```bash
# Validate all datasets
python validate_calibration_datasets.py

# Validate specific dataset with verbose output
python validate_calibration_datasets.py --dataset-path calibration_datasets/cam_april --verbose
```

### Run Calibration Tests
```bash
# Test all datasets
python test_calibration_suite.py

# Test specific dataset
python test_calibration_suite.py --dataset-path calibration_datasets/cam_april --output-dir results_april
```

### Process ROSBAG Data
```bash
# Analyze ROSBAG structure
python rosbag_processor.py --input cam_april.bag --output analysis --action analyze

# Extract images from specific topics
python rosbag_processor.py --input cam_april.bag --output extracted --action extract_images --topics /cam0/image_raw /cam1/image_raw

# Generate complete calibration dataset
python rosbag_processor.py --input cam_april.bag --output new_dataset --action generate_dataset --config custom_config.yaml
```

### Accuracy Testing
```bash
# Comprehensive accuracy evaluation
python calibration_accuracy_tests.py --calibration-results my_calibration.yaml --test-data calibration_datasets/cam_april --output-dir accuracy_report
```

## Configuration Examples

### Custom Dataset Generation Config
```yaml
# custom_config.yaml
dataset_name: 'my_calibration'
target_type: 'aprilgrid'
tagCols: 6
tagRows: 6
tagSize: 0.088
tagSpacing: 0.3
image_topics: ['/cam0/image_raw', '/cam1/image_raw']
imu_topics: ['/imu0/data']
camera_rate: 20
camera_resolution: [752, 480]
imu_rate: 200
camera_extrinsics:
  cam0: [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0]
  cam1: [1.0, 0.0, 0.0, 0.12, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0]
imu_extrinsics:
  imu0: [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0]
```

### Ground Truth for Accuracy Testing
```yaml
# ground_truth.yaml
camera_matrix:
  - [500.0, 0.0, 376.0]
  - [0.0, 500.0, 240.0]
  - [0.0, 0.0, 1.0]
distortion_coeffs: [-0.1, 0.01, -0.001, 0.0, 0.0]
```

## Quality Metrics

### Calibration Quality Standards
- **Excellent**: Mean reprojection error < 0.5 pixels
- **Good**: Mean reprojection error 0.5-1.0 pixels  
- **Fair**: Mean reprojection error 1.0-2.0 pixels
- **Poor**: Mean reprojection error > 2.0 pixels

### Dataset Quality Requirements
- Minimum 10 calibration views for stable results
- Target detection rate > 50% for reliable calibration
- Frame extraction success rate > 90%
- ROSBAG file integrity verified

### Test Performance Benchmarks
- Target detection: < 50ms per frame
- Calibration estimation: < 5 seconds for typical datasets
- Reprojection error calculation: < 1ms per frame

## Troubleshooting

### Common Issues

#### "No YAML config files found"
- Ensure configuration files have `.yaml` extension
- Check file permissions and accessibility
- Verify YAML syntax using online validator

#### "ROSBAG file does not exist"
- Confirm ROSBAG file path is correct
- Check file extension is `.bag`
- Verify file is not corrupted

#### "Low detection rate"
- Check target configuration parameters
- Ensure proper lighting conditions in images
- Verify target type matches actual calibration pattern

#### "Calibration failed"
- Ensure sufficient number of calibration views
- Check for consistent target detection
- Verify image quality and resolution

### Performance Optimization

#### Memory Usage
- Process datasets in chunks for large ROSBAG files
- Limit frame extraction with `--max-frames` parameter
- Use SSD storage for faster I/O operations

#### Processing Speed
- Use multi-processing for frame extraction
- Enable OpenCV optimizations
- Consider GPU acceleration for detection algorithms

## Integration with Main Project

### Using Calibration Results
```python
# Load calibration parameters
import yaml

with open('calibration_results.yaml', 'r') as f:
    calib_data = yaml.safe_load(f)

camera_matrix = np.array(calib_data['camera_matrix'])
distortion_coeffs = np.array(calib_data['distortion_coeffs'])

# Use in perception pipeline
undistorted = cv2.undistort(image, camera_matrix, distortion_coeffs)
```

### Continuous Validation
```bash
# Add to CI/CD pipeline
python validate_calibration_datasets.py --dataset-path calibration_datasets
python test_calibration_suite.py --dataset-path calibration_datasets
```

## Contributing

### Adding New Datasets
1. Follow the established directory structure
2. Include proper YAML configuration files
3. Validate with `validate_calibration_datasets.py`
4. Test with `test_calibration_suite.py`

### Extending Tools
- Add new target types to detection algorithms
- Support additional ROS message types
- Implement new accuracy metrics
- Add visualization capabilities

## References

- [OpenCV Camera Calibration](https://docs.opencv.org/master/d9/d0c/group__calib3d.html)
- [ROS Bag Format](http://wiki.ros.org/rosbag/Format/2.0)
- [AprilTag Library](https://april.eecs.umich.edu/software/apriltag.html)
- [Kalibr Calibration Toolbox](https://github.com/ethz-asl/kalibr)

## License

This calibration dataset and tooling is part of the Mini Autonomy System project. See the main project LICENSE file for details.
