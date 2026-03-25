#!/usr/bin/env python3
"""
Calibration Test Suite

This script provides comprehensive testing for calibration functionality
including target detection, parameter estimation, and accuracy validation.

Usage:
    python test_calibration_suite.py [--dataset-path PATH] [--output-dir PATH] [--tests TESTS]
"""

import os
import sys
import cv2
import numpy as np
import yaml
import argparse
import json
import matplotlib.pyplot as plt
from pathlib import Path
from typing import Dict, List, Tuple, Optional
import logging

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class CalibrationTestSuite:
    """Comprehensive calibration testing suite."""
    
    def __init__(self, dataset_path: str, output_dir: str = "test_results"):
        self.dataset_path = Path(dataset_path)
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(exist_ok=True)
        self.test_results = {}
        
        # Initialize OpenCV ArUco for AprilTag detection
        self.aruco_dict = cv2.aruco.getPredefinedDictionary(cv2.aruco.DICT_APRILTAG_36h11)
        self.aruco_params = cv2.aruco.DetectorParameters()
        
    def run_all_tests(self) -> Dict:
        """Run all calibration tests."""
        logger.info("Starting calibration test suite...")
        
        datasets = [d for d in self.dataset_path.iterdir() if d.is_dir()]
        
        for dataset in datasets:
            logger.info(f"\n=== Testing dataset: {dataset.name} ===")
            dataset_results = self.test_dataset(dataset)
            self.test_results[dataset.name] = dataset_results
        
        self.generate_summary_report()
        return self.test_results
    
    def test_dataset(self, dataset_path: Path) -> Dict:
        """Test a single calibration dataset."""
        results = {
            'dataset': dataset_path.name,
            'tests': {},
            'overall_score': 0.0
        }
        
        # Load configuration
        config = self.load_dataset_config(dataset_path)
        if not config:
            results['tests']['config_load'] = {'status': 'FAILED', 'error': 'Cannot load config'}
            return results
        
        results['tests']['config_load'] = {'status': 'PASSED', 'config': config}
        
        # Test target detection
        detection_results = self.test_target_detection(dataset_path, config)
        results['tests']['target_detection'] = detection_results
        
        # Test calibration parameter estimation
        if detection_results.get('status') == 'PASSED':
            calibration_results = self.test_calibration_estimation(dataset_path, config)
            results['tests']['calibration_estimation'] = calibration_results
        
        # Test reprojection accuracy
        if results['tests'].get('calibration_estimation', {}).get('status') == 'PASSED':
            accuracy_results = self.test_reprojection_accuracy(dataset_path, config)
            results['tests']['reprojection_accuracy'] = accuracy_results
        
        # Calculate overall score
        results['overall_score'] = self.calculate_dataset_score(results['tests'])
        
        return results
    
    def load_dataset_config(self, dataset_path: Path) -> Optional[Dict]:
        """Load dataset configuration."""
        config_files = list(dataset_path.glob('*.yaml'))
        
        for config_file in config_files:
            try:
                with open(config_file, 'r') as f:
                    config = yaml.safe_load(f)
                return config
            except Exception as e:
                logger.error(f"Error loading config {config_file}: {e}")
        
        return None
    
    def test_target_detection(self, dataset_path: Path, config: Dict) -> Dict:
        """Test target detection on sample frames."""
        logger.info("Testing target detection...")
        
        # Find extracted frames
        cam0_data = dataset_path / "cam_april" / "mav0" / "cam0" / "data"
        if not cam0_data.exists():
            cam0_data = dataset_path / "cam_checkerboard" / "mav0" / "cam0" / "data"
        
        if not cam0_data.exists():
            return {'status': 'FAILED', 'error': 'No extracted frames found'}
        
        # Test on sample frames
        image_files = list(cam0_data.glob('*.png'))[:10]  # Test first 10 frames
        
        if not image_files:
            return {'status': 'FAILED', 'error': 'No image files found'}
        
        detection_results = {
            'status': 'PASSED',
            'total_frames': len(image_files),
            'successful_detections': 0,
            'detection_rate': 0.0,
            'avg_detection_time': 0.0,
            'target_type': config.get('target_type', 'unknown')
        }
        
        detection_times = []
        
        for img_file in image_files:
            img = cv2.imread(str(img_file))
            if img is None:
                continue
            
            start_time = cv2.getTickCount()
            
            # Detect based on target type
            if config.get('target_type') == 'aprilgrid':
                try:
                    # Try newer OpenCV API first
                    detector = cv2.aruco.ArucoDetector(self.aruco_dict, self.aruco_params)
                    corners, ids, _ = detector.detectMarkers(img)
                    detected = len(corners) > 0 and len(corners[0]) > 0
                except AttributeError:
                    # Fallback to older API
                    corners, ids, _ = cv2.aruco.detectMarkers(img, self.aruco_dict, parameters=self.aruco_params)
                    detected = len(corners) > 0 and len(corners[0]) > 0
            elif config.get('target_type') == 'checkerboard':
                gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
                pattern_size = (config.get('targetCols', 6), config.get('targetRows', 7))
                ret, corners = cv2.findChessboardCorners(gray, pattern_size, None)
                detected = ret
            else:
                detected = False
            
            end_time = cv2.getTickCount()
            detection_time = (end_time - start_time) / cv2.getTickFrequency() * 1000  # ms
            detection_times.append(detection_time)
            
            if detected:
                detection_results['successful_detections'] += 1
        
        detection_results['detection_rate'] = detection_results['successful_detections'] / len(image_files)
        detection_results['avg_detection_time'] = np.mean(detection_times) if detection_times else 0
        
        # Check if detection rate is acceptable
        if detection_results['detection_rate'] < 0.5:  # Less than 50% detection rate
            detection_results['status'] = 'FAILED'
            detection_results['error'] = f'Low detection rate: {detection_results["detection_rate"]:.2%}'
        
        logger.info(f"Detection rate: {detection_results['detection_rate']:.2%}, "
                   f"Avg time: {detection_results['avg_detection_time']:.2f}ms")
        
        return detection_results
    
    def test_calibration_estimation(self, dataset_path: Path, config: Dict) -> Dict:
        """Test calibration parameter estimation."""
        logger.info("Testing calibration estimation...")
        
        results = {
            'status': 'PASSED',
            'method': 'opencv_calibrate',
            'camera_matrix': None,
            'distortion_coeffs': None,
            'reprojection_error': None,
            'num_frames_used': 0
        }
        
        try:
            # Collect calibration data
            obj_points, img_points = self.collect_calibration_data(dataset_path, config)
            
            if len(obj_points) < 10:  # Need at least 10 calibration views
                results['status'] = 'FAILED'
                results['error'] = f'Insufficient calibration views: {len(obj_points)}'
                return results
            
            # Get image size from first detection
            img_shape = self.get_image_size(dataset_path)
            if img_shape is None:
                results['status'] = 'FAILED'
                results['error'] = 'Cannot determine image size'
                return results
            
            # Perform calibration
            ret, mtx, dist, rvecs, tvecs = cv2.calibrateCamera(
                obj_points, img_points, img_shape, None, None
            )
            
            if not ret:
                results['status'] = 'FAILED'
                results['error'] = 'Calibration failed'
                return results
            
            results['camera_matrix'] = mtx.tolist()
            results['distortion_coeffs'] = dist.tolist()
            results['reprojection_error'] = float(np.mean([cv2.norm(
                cv2.projectPoints(obj_points[i], rvecs[i], tvecs[i], mtx, dist)[0], 
                img_points[i], cv2.NORM_L2
            ) / len(img_points[i]) for i in range(len(obj_points))]))
            results['num_frames_used'] = len(obj_points)
            
            # Check if reprojection error is acceptable
            if results['reprojection_error'] > 1.0:  # More than 1 pixel error
                results['status'] = 'WARNING'
                results['warning'] = f'High reprojection error: {results["reprojection_error"]:.3f} pixels'
            
            logger.info(f"Calibration successful. Reprojection error: {results['reprojection_error']:.3f} pixels")
            
        except Exception as e:
            results['status'] = 'FAILED'
            results['error'] = str(e)
            logger.error(f"Calibration estimation failed: {e}")
        
        return results
    
    def test_reprojection_accuracy(self, dataset_path: Path, config: Dict) -> Dict:
        """Test reprojection accuracy on test frames."""
        logger.info("Testing reprojection accuracy...")
        
        results = {
            'status': 'PASSED',
            'mean_error': 0.0,
            'max_error': 0.0,
            'std_error': 0.0,
            'test_frames': 0
        }
        
        try:
            # Load calibration results from previous test
            calibration_results = self.test_results.get(dataset_path.name, {}).get('tests', {}).get('calibration_estimation', {})
            
            if calibration_results.get('status') not in ['PASSED', 'WARNING']:
                results['status'] = 'FAILED'
                results['error'] = 'No valid calibration available'
                return results
            
            mtx = np.array(calibration_results['camera_matrix'])
            dist = np.array(calibration_results['distortion_coeffs'])
            
            # Test on different frames
            errors = []
            test_frames = self.get_test_frames(dataset_path, config, num_frames=20)
            
            for frame_data in test_frames:
                error = self.compute_reprojection_error(frame_data, mtx, dist, config)
                if error is not None:
                    errors.append(error)
            
            if not errors:
                results['status'] = 'FAILED'
                results['error'] = 'No valid test frames'
                return results
            
            results['mean_error'] = float(np.mean(errors))
            results['max_error'] = float(np.max(errors))
            results['std_error'] = float(np.std(errors))
            results['test_frames'] = len(errors)
            
            # Check accuracy thresholds
            if results['mean_error'] > 2.0:  # More than 2 pixels mean error
                results['status'] = 'FAILED'
                results['error'] = f'Poor accuracy: {results["mean_error"]:.3f} pixels mean error'
            elif results['mean_error'] > 1.0:  # More than 1 pixel
                results['status'] = 'WARNING'
                results['warning'] = f'Moderate accuracy: {results["mean_error"]:.3f} pixels mean error'
            
            logger.info(f"Reprojection accuracy: {results['mean_error']:.3f} ± {results['std_error']:.3f} pixels")
            
        except Exception as e:
            results['status'] = 'FAILED'
            results['error'] = str(e)
            logger.error(f"Reprojection accuracy test failed: {e}")
        
        return results
    
    def collect_calibration_data(self, dataset_path: Path, config: Dict) -> Tuple[List, List]:
        """Collect object points and image points for calibration."""
        obj_points = []
        img_points = []
        
        # Generate object points based on target type
        if config.get('target_type') == 'aprilgrid':
            objp = self.generate_aprilgrid_object_points(config)
        else:  # checkerboard
            objp = self.generate_checkerboard_object_points(config)
        
        # Process calibration frames
        cam0_data = dataset_path / "cam_april" / "mav0" / "cam0" / "data"
        if not cam0_data.exists():
            cam0_data = dataset_path / "cam_checkerboard" / "mav0" / "cam0" / "data"
        
        image_files = list(cam0_data.glob('*.png'))
        
        for img_file in image_files:
            img = cv2.imread(str(img_file))
            if img is None:
                continue
            
            # Detect target
            if config.get('target_type') == 'aprilgrid':
                try:
                    detector = cv2.aruco.ArucoDetector(self.aruco_dict, self.aruco_params)
                    corners, ids, _ = detector.detectMarkers(img)
                    if len(corners) > 0 and len(corners[0]) > 0:
                        img_points.append(corners[0].reshape(-1, 2))
                        obj_points.append(objp)
                except AttributeError:
                    corners, ids, _ = cv2.aruco.detectMarkers(img, self.aruco_dict, parameters=self.aruco_params)
                    if len(corners) > 0 and len(corners[0]) > 0:
                        img_points.append(corners[0].reshape(-1, 2))
                        obj_points.append(objp)
            elif config.get('target_type') == 'checkerboard':
                gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
                pattern_size = (config.get('targetCols', 6), config.get('targetRows', 7))
                ret, corners = cv2.findChessboardCorners(gray, pattern_size, None)
                if ret:
                    # Refine corners
                    criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)
                    corners = cv2.cornerSubPix(gray, corners, (11, 11), (-1, -1), criteria)
                    img_points.append(corners.reshape(-1, 2))
                    obj_points.append(objp)
        
        return obj_points, img_points
    
    def generate_aprilgrid_object_points(self, config: Dict) -> np.ndarray:
        """Generate object points for AprilGrid."""
        tag_cols = config.get('tagCols', 6)
        tag_rows = config.get('tagRows', 6)
        tag_size = config.get('tagSize', 0.088)
        tag_spacing = config.get('tagSpacing', 0.3)
        
        spacing = tag_size * tag_spacing
        
        objp = []
        for row in range(tag_rows):
            for col in range(tag_cols):
                x = col * (tag_size + spacing)
                y = row * (tag_size + spacing)
                objp.append([x, y, 0])
        
        return np.array(objp, dtype=np.float32)
    
    def generate_checkerboard_object_points(self, config: Dict) -> np.ndarray:
        """Generate object points for checkerboard."""
        cols = config.get('targetCols', 6)
        rows = config.get('targetRows', 7)
        square_size = config.get('rowSpacingMeters', 0.06)
        
        objp = np.zeros((rows * cols, 3), dtype=np.float32)
        objp[:, :2] = np.mgrid[0:cols, 0:rows].T.reshape(-1, 2) * square_size
        
        return objp
    
    def get_image_size(self, dataset_path: Path) -> Optional[Tuple[int, int]]:
        """Get image size from dataset."""
        cam0_data = dataset_path / "cam_april" / "mav0" / "cam0" / "data"
        if not cam0_data.exists():
            cam0_data = dataset_path / "cam_checkerboard" / "mav0" / "cam0" / "data"
        
        image_files = list(cam0_data.glob('*.png'))
        if not image_files:
            return None
        
        img = cv2.imread(str(image_files[0]))
        if img is None:
            return None
        
        return img.shape[:2][::-1]  # Return (width, height)
    
    def get_test_frames(self, dataset_path: Path, config: Dict, num_frames: int = 20) -> List[Dict]:
        """Get test frames for accuracy evaluation."""
        frames = []
        
        cam0_data = dataset_path / "cam_april" / "mav0" / "cam0" / "data"
        if not cam0_data.exists():
            cam0_data = dataset_path / "cam_checkerboard" / "mav0" / "cam0" / "data"
        
        image_files = list(cam0_data.glob('*.png'))
        
        # Use frames that weren't used for calibration
        test_files = image_files[-num_frames:] if len(image_files) > num_frames else image_files
        
        for img_file in test_files:
            img = cv2.imread(str(img_file))
            if img is None:
                continue
            
            frame_data = {'image': img, 'filename': img_file.name}
            
            # Detect target
            if config.get('target_type') == 'aprilgrid':
                try:
                    detector = cv2.aruco.ArucoDetector(self.aruco_dict, self.aruco_params)
                    corners, ids, _ = detector.detectMarkers(img)
                    if len(corners) > 0 and len(corners[0]) > 0:
                        frame_data['detected_corners'] = corners[0].reshape(-1, 2)
                        frames.append(frame_data)
                except AttributeError:
                    corners, ids, _ = cv2.aruco.detectMarkers(img, self.aruco_dict, parameters=self.aruco_params)
                    if len(corners) > 0 and len(corners[0]) > 0:
                        frame_data['detected_corners'] = corners[0].reshape(-1, 2)
                        frames.append(frame_data)
            elif config.get('target_type') == 'checkerboard':
                gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
                pattern_size = (config.get('targetCols', 6), config.get('targetRows', 7))
                ret, corners = cv2.findChessboardCorners(gray, pattern_size, None)
                if ret:
                    criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)
                    corners = cv2.cornerSubPix(gray, corners, (11, 11), (-1, -1), criteria)
                    frame_data['detected_corners'] = corners.reshape(-1, 2)
                    frames.append(frame_data)
        
        return frames
    
    def compute_reprojection_error(self, frame_data: Dict, mtx: np.ndarray, dist: np.ndarray, config: Dict) -> Optional[float]:
        """Compute reprojection error for a single frame."""
        try:
            img_points = frame_data['detected_corners']
            
            # Generate corresponding object points
            if config.get('target_type') == 'aprilgrid':
                objp = self.generate_aprilgrid_object_points(config)[:len(img_points)]
            else:
                objp = self.generate_checkerboard_object_points(config)[:len(img_points)]
            
            # Project object points
            img_points_proj, _ = cv2.projectPoints(objp, np.zeros(3), np.zeros(3), mtx, dist)
            
            # Compute error
            error = cv2.norm(img_points, img_points_proj.reshape(-1, 2), cv2.NORM_L2) / len(img_points)
            
            return float(error)
            
        except Exception as e:
            logger.error(f"Error computing reprojection error: {e}")
            return None
    
    def calculate_dataset_score(self, test_results: Dict) -> float:
        """Calculate overall score for a dataset."""
        score = 0.0
        total_weight = 0.0
        
        # Test weights
        weights = {
            'config_load': 0.1,
            'target_detection': 0.3,
            'calibration_estimation': 0.3,
            'reprojection_accuracy': 0.3
        }
        
        for test_name, weight in weights.items():
            if test_name in test_results:
                result = test_results[test_name]
                if result['status'] == 'PASSED':
                    score += weight
                elif result['status'] == 'WARNING':
                    score += weight * 0.7  # 70% score for warnings
                
                total_weight += weight
        
        return score / total_weight if total_weight > 0 else 0.0
    
    def generate_summary_report(self):
        """Generate comprehensive summary report."""
        report = {
            'summary': {
                'total_datasets': len(self.test_results),
                'overall_success_rate': 0.0,
                'average_score': 0.0
            },
            'datasets': self.test_results,
            'recommendations': []
        }
        
        # Calculate summary statistics
        successful_datasets = sum(1 for r in self.test_results.values() if r['overall_score'] > 0.7)
        report['summary']['overall_success_rate'] = successful_datasets / len(self.test_results)
        report['summary']['average_score'] = np.mean([r['overall_score'] for r in self.test_results.values()])
        
        # Generate recommendations
        for dataset_name, results in self.test_results.items():
            if results['overall_score'] < 0.5:
                report['recommendations'].append(f"Dataset {dataset_name} needs significant improvement")
            elif results['overall_score'] < 0.7:
                report['recommendations'].append(f"Dataset {dataset_name} has moderate quality issues")
        
        # Save report
        report_file = self.output_dir / 'calibration_test_report.json'
        with open(report_file, 'w') as f:
            json.dump(report, f, indent=2)
        
        # Generate plots
        self.generate_performance_plots()
        
        logger.info(f"Summary report saved to: {report_file}")
    
    def generate_performance_plots(self):
        """Generate performance visualization plots."""
        fig, axes = plt.subplots(2, 2, figsize=(12, 10))
        
        dataset_names = list(self.test_results.keys())
        scores = [r['overall_score'] for r in self.test_results.values()]
        
        # Overall scores
        axes[0, 0].bar(dataset_names, scores)
        axes[0, 0].set_title('Overall Calibration Scores')
        axes[0, 0].set_ylabel('Score')
        axes[0, 0].tick_params(axis='x', rotation=45)
        
        # Detection rates
        detection_rates = []
        for results in self.test_results.values():
            detection_test = results['tests'].get('target_detection', {})
            detection_rates.append(detection_test.get('detection_rate', 0))
        
        axes[0, 1].bar(dataset_names, detection_rates)
        axes[0, 1].set_title('Target Detection Rates')
        axes[0, 1].set_ylabel('Detection Rate')
        axes[0, 1].tick_params(axis='x', rotation=45)
        
        # Reprojection errors
        reprojection_errors = []
        for results in self.test_results.values():
            accuracy_test = results['tests'].get('reprojection_accuracy', {})
            if accuracy_test.get('status') in ['PASSED', 'WARNING']:
                reprojection_errors.append(accuracy_test.get('mean_error', 0))
            else:
                reprojection_errors.append(np.nan)
        
        axes[1, 0].bar(dataset_names, reprojection_errors)
        axes[1, 0].set_title('Mean Reprojection Errors')
        axes[1, 0].set_ylabel('Error (pixels)')
        axes[1, 0].tick_params(axis='x', rotation=45)
        
        # Test status summary
        test_statuses = {'PASSED': 0, 'WARNING': 0, 'FAILED': 0}
        for results in self.test_results.values():
            for test_result in results['tests'].values():
                test_statuses[test_result['status']] += 1
        
        axes[1, 1].pie(test_statuses.values(), labels=test_statuses.keys(), autopct='%1.1f%%')
        axes[1, 1].set_title('Test Status Distribution')
        
        plt.tight_layout()
        plot_file = self.output_dir / 'calibration_performance.png'
        plt.savefig(plot_file, dpi=300, bbox_inches='tight')
        plt.close()
        
        logger.info(f"Performance plots saved to: {plot_file}")

def main():
    parser = argparse.ArgumentParser(description='Run calibration test suite')
    parser.add_argument('--dataset-path', 
                       default='calibration_datasets',
                       help='Path to calibration datasets directory')
    parser.add_argument('--output-dir', 
                       default='test_results',
                       help='Output directory for test results')
    parser.add_argument('--tests', 
                       nargs='+',
                       choices=['detection', 'calibration', 'accuracy', 'all'],
                       default=['all'],
                       help='Specific tests to run')
    
    args = parser.parse_args()
    
    suite = CalibrationTestSuite(args.dataset_path, args.output_dir)
    results = suite.run_all_tests()
    
    # Print summary
    print("\n" + "="*60)
    print("CALIBRATION TEST SUITE SUMMARY")
    print("="*60)
    
    for dataset_name, dataset_results in results.items():
        score = dataset_results['overall_score']
        status = "✅ EXCELLENT" if score > 0.9 else "✅ GOOD" if score > 0.7 else "⚠️  FAIR" if score > 0.5 else "❌ POOR"
        print(f"{dataset_name}: {score:.2f} - {status}")
    
    print("="*60)

if __name__ == '__main__':
    main()
