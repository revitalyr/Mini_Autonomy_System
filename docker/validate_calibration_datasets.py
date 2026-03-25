#!/usr/bin/env python3
"""
Calibration Dataset Validation Script

This script validates the structure and integrity of calibration datasets
in the docker/calibration_datasets directory. It checks for:
- Required YAML configuration files
- ROSBAG file format and accessibility
- CSV data files and structure
- Image frame extraction (data folders)
- Configuration consistency

Usage:
    python validate_calibration_datasets.py [--dataset-path PATH] [--verbose]
"""

import os
import sys
import yaml
import argparse
import pandas as pd
from pathlib import Path
from typing import Dict, List, Optional, Tuple
import logging

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class CalibrationDatasetValidator:
    """Validates calibration dataset structure and content."""
    
    REQUIRED_FILES = {
        'config_yaml': ['*.yaml'],
        'rosbag': ['*.bag'],
        'sensor_data': ['sensor.yaml', 'data.csv'],
        'extracted_frames': ['data/']
    }
    
    TARGET_TYPES = ['aprilgrid', 'checkerboard']
    
    def __init__(self, dataset_path: str, verbose: bool = False):
        self.dataset_path = Path(dataset_path)
        self.verbose = verbose
        self.errors = []
        self.warnings = []
        
        if verbose:
            logger.setLevel(logging.DEBUG)
    
    def validate_all(self) -> bool:
        """Validate all datasets in the directory."""
        logger.info(f"Validating calibration datasets in: {self.dataset_path}")
        
        if not self.dataset_path.exists():
            self.errors.append(f"Dataset path does not exist: {self.dataset_path}")
            return False
        
        # Find all dataset subdirectories
        datasets = [d for d in self.dataset_path.iterdir() if d.is_dir()]
        
        if not datasets:
            self.errors.append("No dataset subdirectories found")
            return False
        
        logger.info(f"Found {len(datasets)} datasets: {[d.name for d in datasets]}")
        
        overall_success = True
        for dataset in datasets:
            logger.info(f"\n=== Validating dataset: {dataset.name} ===")
            success = self.validate_single_dataset(dataset)
            overall_success = overall_success and success
        
        self.print_summary()
        return overall_success
    
    def validate_single_dataset(self, dataset_path: Path) -> bool:
        """Validate a single dataset directory."""
        success = True
        
        # Check main configuration files
        config_files = list(dataset_path.glob('*.yaml'))
        if not config_files:
            self.errors.append(f"No YAML config files found in {dataset_path.name}")
            success = False
        else:
            for config_file in config_files:
                if not self.validate_config_yaml(config_file):
                    success = False
        
        # Check ROSBAG file
        rosbag_files = list(dataset_path.glob('*.bag'))
        if not rosbag_files:
            self.errors.append(f"No ROSBAG file found in {dataset_path.name}")
            success = False
        else:
            for rosbag_file in rosbag_files:
                if not self.validate_rosbag(rosbag_file):
                    success = False
        
        # Check extracted dataset structure
        extracted_dirs = [d for d in dataset_path.iterdir() if d.is_dir() and d.name != dataset_path.name]
        for extracted_dir in extracted_dirs:
            if not self.validate_extracted_dataset(extracted_dir):
                success = False
        
        return success
    
    def validate_config_yaml(self, config_file: Path) -> bool:
        """Validate YAML configuration file."""
        logger.debug(f"Validating config file: {config_file}")
        
        try:
            with open(config_file, 'r') as f:
                config = yaml.safe_load(f)
            
            if not config:
                self.errors.append(f"Empty config file: {config_file}")
                return False
            
            # Check target type
            target_type = config.get('target_type')
            if not target_type:
                self.errors.append(f"Missing target_type in {config_file}")
                return False
            
            if target_type not in self.TARGET_TYPES:
                self.errors.append(f"Invalid target_type '{target_type}' in {config_file}. "
                                 f"Expected: {self.TARGET_TYPES}")
                return False
            
            # Validate target-specific parameters
            if target_type == 'aprilgrid':
                required_params = ['tagCols', 'tagRows', 'tagSize', 'tagSpacing']
            else:  # checkerboard
                required_params = ['targetCols', 'targetRows', 'rowSpacingMeters', 'colSpacingMeters']
            
            for param in required_params:
                if param not in config:
                    self.errors.append(f"Missing required parameter '{param}' in {config_file}")
                    return False
            
            logger.info(f"✓ Config file valid: {config_file}")
            return True
            
        except yaml.YAMLError as e:
            self.errors.append(f"Invalid YAML in {config_file}: {e}")
            return False
        except Exception as e:
            self.errors.append(f"Error reading {config_file}: {e}")
            return False
    
    def validate_rosbag(self, rosbag_file: Path) -> bool:
        """Validate ROSBAG file."""
        logger.debug(f"Validating ROSBAG file: {rosbag_file}")
        
        if not rosbag_file.exists():
            self.errors.append(f"ROSBAG file does not exist: {rosbag_file}")
            return False
        
        # Check file size (should be substantial)
        file_size_mb = rosbag_file.stat().st_size / (1024 * 1024)
        if file_size_mb < 1:  # Less than 1MB seems suspicious
            self.warnings.append(f"ROSBAG file seems small: {rosbag_file} ({file_size_mb:.2f} MB)")
        
        # Try to read ROSBAG header (basic validation)
        try:
            with open(rosbag_file, 'rb') as f:
                header = f.read(10)
                # Check for ROSBAG V2.0 magic number
                if not header.startswith(b'#ROSBAG'):
                    self.errors.append(f"Invalid ROSBAG format: {rosbag_file}")
                    return False
            
            logger.info(f"✓ ROSBAG file valid: {rosbag_file} ({file_size_mb:.2f} MB)")
            return True
            
        except Exception as e:
            self.errors.append(f"Error reading ROSBAG {rosbag_file}: {e}")
            return False
    
    def validate_extracted_dataset(self, extracted_dir: Path) -> bool:
        """Validate extracted dataset structure (e.g., mav0/)."""
        logger.debug(f"Validating extracted dataset: {extracted_dir}")
        
        success = True
        
        # Check sensor subdirectories
        sensor_dirs = [d for d in extracted_dir.iterdir() if d.is_dir()]
        expected_sensors = ['cam0', 'cam1', 'imu0']
        
        for sensor_dir in sensor_dirs:
            if not self.validate_sensor_dir(sensor_dir):
                success = False
        
        logger.info(f"✓ Extracted dataset valid: {extracted_dir}")
        return success
    
    def validate_sensor_dir(self, sensor_dir: Path) -> bool:
        """Validate sensor directory (cam0, cam1, imu0)."""
        logger.debug(f"Validating sensor directory: {sensor_dir}")
        
        success = True
        
        # Check sensor.yaml
        sensor_yaml = sensor_dir / 'sensor.yaml'
        if not sensor_yaml.exists():
            self.errors.append(f"Missing sensor.yaml in {sensor_dir}")
            success = False
        else:
            if not self.validate_sensor_yaml(sensor_yaml):
                success = False
        
        # Check data.csv
        data_csv = sensor_dir / 'data.csv'
        if not data_csv.exists():
            self.errors.append(f"Missing data.csv in {sensor_dir}")
            success = False
        else:
            if not self.validate_data_csv(data_csv):
                success = False
        
        # Check data/ directory with extracted frames
        data_dir = sensor_dir / 'data'
        if data_dir.exists():
            if not self.validate_extracted_frames(data_dir):
                success = False
        
        return success
    
    def validate_sensor_yaml(self, sensor_yaml: Path) -> bool:
        """Validate sensor configuration YAML."""
        try:
            with open(sensor_yaml, 'r') as f:
                config = yaml.safe_load(f)
            
            sensor_type = config.get('sensor_type')
            if not sensor_type:
                self.errors.append(f"Missing sensor_type in {sensor_yaml}")
                return False
            
            # Check for required transformation matrix
            t_bs = config.get('T_BS')
            if not t_bs or 'data' not in t_bs:
                self.errors.append(f"Missing T_BS transformation matrix in {sensor_yaml}")
                return False
            
            logger.debug(f"✓ Sensor YAML valid: {sensor_yaml}")
            return True
            
        except Exception as e:
            self.errors.append(f"Error reading sensor YAML {sensor_yaml}: {e}")
            return False
    
    def validate_data_csv(self, data_csv: Path) -> bool:
        """Validate sensor data CSV file."""
        try:
            df = pd.read_csv(data_csv)
            
            if df.empty:
                self.errors.append(f"Empty data CSV: {data_csv}")
                return False
            
            # Check for required columns (timestamp and data)
            if len(df.columns) < 2:
                self.errors.append(f"Insufficient columns in {data_csv}")
                return False
            
            # Check timestamp format (should be numeric)
            timestamp_col = df.columns[0]
            if not pd.api.types.is_numeric_dtype(df[timestamp_col]):
                self.errors.append(f"Invalid timestamp format in {data_csv}")
                return False
            
            logger.debug(f"✓ Data CSV valid: {data_csv} ({len(df)} rows)")
            return True
            
        except Exception as e:
            self.errors.append(f"Error reading data CSV {data_csv}: {e}")
            return False
    
    def validate_extracted_frames(self, data_dir: Path) -> bool:
        """Validate extracted image frames."""
        image_files = list(data_dir.glob('*.png')) + list(data_dir.glob('*.jpg'))
        
        if not image_files:
            self.warnings.append(f"No image frames found in {data_dir}")
            return True  # Not an error, frames might not be extracted
        
        # Check file sizes
        for img_file in image_files[:5]:  # Check first 5 files
            if img_file.stat().st_size < 1000:  # Less than 1KB seems suspicious
                self.warnings.append(f"Image file seems small: {img_file}")
        
        logger.debug(f"✓ Found {len(image_files)} image frames in {data_dir}")
        return True
    
    def print_summary(self):
        """Print validation summary."""
        print("\n" + "="*60)
        print("CALIBRATION DATASET VALIDATION SUMMARY")
        print("="*60)
        
        if self.errors:
            print(f"\n❌ ERRORS ({len(self.errors)}):")
            for error in self.errors:
                print(f"  • {error}")
        
        if self.warnings:
            print(f"\n⚠️  WARNINGS ({len(self.warnings)}):")
            for warning in self.warnings:
                print(f"  • {warning}")
        
        if not self.errors and not self.warnings:
            print("\n✅ All datasets validated successfully!")
        elif not self.errors:
            print("\n✅ All datasets valid (with warnings)")
        else:
            print(f"\n❌ Validation failed with {len(self.errors)} error(s)")
        
        print("="*60)

def main():
    parser = argparse.ArgumentParser(description='Validate calibration datasets')
    parser.add_argument('--dataset-path', 
                       default='calibration_datasets',
                       help='Path to calibration datasets directory')
    parser.add_argument('--verbose', '-v', 
                       action='store_true',
                       help='Enable verbose output')
    
    args = parser.parse_args()
    
    validator = CalibrationDatasetValidator(args.dataset_path, args.verbose)
    success = validator.validate_all()
    
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()
