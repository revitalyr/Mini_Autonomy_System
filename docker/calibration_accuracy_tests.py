#!/usr/bin/env python3
"""
Calibration Accuracy Tests

This script provides comprehensive accuracy testing for calibration results,
including statistical analysis, cross-validation, and benchmarking against
known ground truth data.

Usage:
    python calibration_accuracy_tests.py --calibration-results CALIB_FILE --test-data DATA_DIR
"""

import os
import sys
import cv2
import numpy as np
import yaml
import json
import argparse
from pathlib import Path
from typing import Dict, List, Tuple, Optional, Any
import logging
from scipy import stats
import matplotlib.pyplot as plt
from sklearn.metrics import mean_squared_error
import seaborn as sns

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class CalibrationAccuracyTester:
    """Comprehensive calibration accuracy testing."""
    
    def __init__(self, calibration_file: str, test_data_dir: str, output_dir: str = "accuracy_results"):
        self.calibration_file = Path(calibration_file)
        self.test_data_dir = Path(test_data_dir)
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(exist_ok=True)
        
        self.calibration_data = self.load_calibration_data()
        self.test_results = {}
        
        # Initialize OpenCV ArUco for AprilTag detection
        self.aruco_dict = cv2.aruco.getPredefinedDictionary(cv2.aruco.DICT_APRILTAG_36h11)
        self.aruco_params = cv2.aruco.DetectorParameters()
    
    def load_calibration_data(self) -> Optional[Dict]:
        """Load calibration parameters."""
        try:
            with open(self.calibration_file, 'r') as f:
                calib_data = yaml.safe_load(f)
            
            # Validate required fields
            required_fields = ['camera_matrix', 'distortion_coeffs']
            for field in required_fields:
                if field not in calib_data:
                    logger.error(f"Missing required calibration field: {field}")
                    return None
            
            # Convert to numpy arrays
            calib_data['camera_matrix'] = np.array(calib_data['camera_matrix'])
            calib_data['distortion_coeffs'] = np.array(calib_data['distortion_coeffs'])
            
            logger.info(f"Loaded calibration data from: {self.calibration_file}")
            return calib_data
            
        except Exception as e:
            logger.error(f"Failed to load calibration data: {e}")
            return None
    
    def run_comprehensive_tests(self) -> Dict:
        """Run all accuracy tests."""
        logger.info("Starting comprehensive calibration accuracy tests...")
        
        if not self.calibration_data:
            logger.error("No valid calibration data available")
            return {}
        
        # Test 1: Reprojection Error Analysis
        reprojection_results = self.test_reprojection_error_analysis()
        self.test_results['reprojection_error_analysis'] = reprojection_results
        
        # Test 2: Cross-Validation Test
        cv_results = self.test_cross_validation()
        self.test_results['cross_validation'] = cv_results
        
        # Test 3: Calibration Stability Test
        stability_results = self.test_calibration_stability()
        self.test_results['calibration_stability'] = stability_results
        
        # Test 4: Ground Truth Comparison (if available)
        gt_results = self.test_ground_truth_comparison()
        self.test_results['ground_truth_comparison'] = gt_results
        
        # Test 5: Statistical Analysis
        stats_results = self.test_statistical_analysis()
        self.test_results['statistical_analysis'] = stats_results
        
        # Generate comprehensive report
        self.generate_accuracy_report()
        
        return self.test_results
    
    def test_reprojection_error_analysis(self) -> Dict:
        """Detailed reprojection error analysis."""
        logger.info("Performing reprojection error analysis...")
        
        results = {
            'status': 'PASSED',
            'mean_error': 0.0,
            'std_error': 0.0,
            'max_error': 0.0,
            'min_error': 0.0,
            'median_error': 0.0,
            'error_distribution': {},
            'outlier_count': 0,
            'outlier_percentage': 0.0
        }
        
        try:
            # Collect all reprojection errors
            all_errors = []
            frame_errors = []
            
            # Process test frames
            test_frames = self.get_test_frames()
            
            for frame_data in test_frames:
                frame_error = self.compute_frame_reprojection_error(frame_data)
                if frame_error is not None:
                    all_errors.extend(frame_error)
                    frame_errors.append(np.mean(frame_error))
            
            if not all_errors:
                results['status'] = 'FAILED'
                results['error'] = 'No valid reprojection errors computed'
                return results
            
            # Compute statistics
            results['mean_error'] = float(np.mean(all_errors))
            results['std_error'] = float(np.std(all_errors))
            results['max_error'] = float(np.max(all_errors))
            results['min_error'] = float(np.min(all_errors))
            results['median_error'] = float(np.median(all_errors))
            
            # Error distribution analysis
            error_bins = [0, 0.5, 1.0, 2.0, 5.0, float('inf')]
            error_labels = ['<0.5px', '0.5-1px', '1-2px', '2-5px', '>5px']
            
            for i, (lower, upper) in enumerate(zip(error_bins[:-1], error_bins[1:])):
                count = sum(1 for e in all_errors if lower <= e < upper)
                results['error_distribution'][error_labels[i]] = {
                    'count': count,
                    'percentage': count / len(all_errors) * 100
                }
            
            # Outlier detection (using 3-sigma rule)
            threshold = results['mean_error'] + 3 * results['std_error']
            results['outlier_count'] = sum(1 for e in all_errors if e > threshold)
            results['outlier_percentage'] = results['outlier_count'] / len(all_errors) * 100
            
            # Quality assessment
            if results['mean_error'] > 2.0:
                results['status'] = 'FAILED'
                results['quality'] = 'POOR'
            elif results['mean_error'] > 1.0:
                results['status'] = 'WARNING'
                results['quality'] = 'FAIR'
            elif results['mean_error'] > 0.5:
                results['quality'] = 'GOOD'
            else:
                results['quality'] = 'EXCELLENT'
            
            # Generate error histogram
            self.plot_error_histogram(all_errors, 'reprojection_error_histogram.png')
            
            logger.info(f"Reprojection error analysis: {results['mean_error']:.3f} ± {results['std_error']:.3f} pixels")
            
        except Exception as e:
            results['status'] = 'FAILED'
            results['error'] = str(e)
            logger.error(f"Reprojection error analysis failed: {e}")
        
        return results
    
    def test_cross_validation(self) -> Dict:
        """Cross-validation test using different data splits."""
        logger.info("Performing cross-validation test...")
        
        results = {
            'status': 'PASSED',
            'folds': 5,
            'fold_scores': [],
            'mean_cv_score': 0.0,
            'std_cv_score': 0.0,
            'score_variance': 0.0
        }
        
        try:
            # Get all available frames
            all_frames = self.get_test_frames()
            
            if len(all_frames) < results['folds']:
                results['status'] = 'FAILED'
                results['error'] = f'Insufficient frames for {results["folds"]}-fold CV: {len(all_frames)}'
                return results
            
            # Perform k-fold cross-validation
            fold_size = len(all_frames) // results['folds']
            fold_scores = []
            
            for fold in range(results['folds']):
                # Split data
                test_start = fold * fold_size
                test_end = (fold + 1) * fold_size if fold < results['folds'] - 1 else len(all_frames)
                
                test_frames = all_frames[test_start:test_end]
                train_frames = all_frames[:test_start] + all_frames[test_end:]
                
                # Calibrate on training data
                calib_result = self.calibrate_on_frames(train_frames)
                
                if calib_result['status'] != 'PASSED':
                    fold_scores.append(0.0)
                    continue
                
                # Test on validation data
                validation_score = self.validate_calibration(calib_result, test_frames)
                fold_scores.append(validation_score)
            
            results['fold_scores'] = fold_scores
            results['mean_cv_score'] = float(np.mean(fold_scores))
            results['std_cv_score'] = float(np.std(fold_scores))
            results['score_variance'] = float(np.var(fold_scores))
            
            # Quality assessment
            if results['mean_cv_score'] < 0.5:
                results['status'] = 'FAILED'
                results['quality'] = 'POOR'
            elif results['mean_cv_score'] < 0.7:
                results['status'] = 'WARNING'
                results['quality'] = 'FAIR'
            else:
                results['quality'] = 'GOOD'
            
            # Generate CV plot
            self.plot_cross_validation_results(fold_scores, 'cross_validation_results.png')
            
            logger.info(f"Cross-validation score: {results['mean_cv_score']:.3f} ± {results['std_cv_score']:.3f}")
            
        except Exception as e:
            results['status'] = 'FAILED'
            results['error'] = str(e)
            logger.error(f"Cross-validation failed: {e}")
        
        return results
    
    def test_calibration_stability(self) -> Dict:
        """Test calibration stability across different subsets."""
        logger.info("Testing calibration stability...")
        
        results = {
            'status': 'PASSED',
            'subsets_tested': 10,
            'parameter_variations': {},
            'stability_score': 0.0,
            'unstable_parameters': []
        }
        
        try:
            # Get all frames
            all_frames = self.get_test_frames()
            
            if len(all_frames) < 20:
                results['status'] = 'FAILED'
                results['error'] = f'Insufficient frames for stability test: {len(all_frames)}'
                return results
            
            subset_size = len(all_frames) // 2
            camera_matrices = []
            distortion_coeffs_list = []
            
            # Test multiple random subsets
            for i in range(results['subsets_tested']):
                # Random subset
                subset_indices = np.random.choice(len(all_frames), subset_size, replace=False)
                subset_frames = [all_frames[j] for j in subset_indices]
                
                # Calibrate on subset
                calib_result = self.calibrate_on_frames(subset_frames)
                
                if calib_result['status'] == 'PASSED':
                    camera_matrices.append(calib_result['camera_matrix'])
                    distortion_coeffs_list.append(calib_result['distortion_coeffs'])
            
            if len(camera_matrices) < 3:
                results['status'] = 'FAILED'
                results['error'] = 'Insufficient successful calibrations for stability analysis'
                return results
            
            # Analyze parameter variations
            camera_matrices = np.array(camera_matrices)
            distortion_coeffs_list = np.array(distortion_coeffs_list)
            
            # Camera matrix stability
            fx_std = np.std(camera_matrices[:, 0, 0])
            fy_std = np.std(camera_matrices[:, 1, 1])
            cx_std = np.std(camera_matrices[:, 0, 2])
            cy_std = np.std(camera_matrices[:, 1, 2])
            
            results['parameter_variations']['fx'] = {'mean': float(np.mean(camera_matrices[:, 0, 0])), 'std': float(fx_std)}
            results['parameter_variations']['fy'] = {'mean': float(np.mean(camera_matrices[:, 1, 1])), 'std': float(fy_std)}
            results['parameter_variations']['cx'] = {'mean': float(np.mean(camera_matrices[:, 0, 2])), 'std': float(cx_std)}
            results['parameter_variations']['cy'] = {'mean': float(np.mean(camera_matrices[:, 1, 2])), 'std': float(cy_std)}
            
            # Distortion coefficients stability
            for i in range(5):  # OpenCV uses 5 distortion coefficients
                param_std = np.std(distortion_coeffs_list[:, i])
                param_mean = np.mean(distortion_coeffs_list[:, i])
                results['parameter_variations'][f'k{i+1}'] = {'mean': float(param_mean), 'std': float(param_std)}
            
            # Calculate stability score (lower variation = higher stability)
            all_stds = [v['std'] for v in results['parameter_variations'].values()]
            mean_std = np.mean(all_stds)
            results['stability_score'] = max(0, 1 - mean_std / 10)  # Normalize to [0,1]
            
            # Identify unstable parameters (> 5% relative variation)
            for param, stats in results['parameter_variations'].items():
                if stats['mean'] != 0 and stats['std'] / abs(stats['mean']) > 0.05:
                    results['unstable_parameters'].append(param)
            
            # Quality assessment
            if results['stability_score'] < 0.5:
                results['status'] = 'FAILED'
                results['quality'] = 'UNSTABLE'
            elif results['stability_score'] < 0.7:
                results['status'] = 'WARNING'
                results['quality'] = 'MODERATELY_STABLE'
            else:
                results['quality'] = 'STABLE'
            
            # Generate stability plot
            self.plot_parameter_stability(results['parameter_variations'], 'parameter_stability.png')
            
            logger.info(f"Calibration stability score: {results['stability_score']:.3f}")
            
        except Exception as e:
            results['status'] = 'FAILED'
            results['error'] = str(e)
            logger.error(f"Calibration stability test failed: {e}")
        
        return results
    
    def test_ground_truth_comparison(self) -> Dict:
        """Compare calibration results with ground truth (if available)."""
        logger.info("Testing ground truth comparison...")
        
        results = {
            'status': 'SKIPPED',
            'ground_truth_available': False,
            'parameter_errors': {},
            'overall_error': 0.0
        }
        
        try:
            # Look for ground truth file
            gt_file = self.test_data_dir / 'ground_truth.yaml'
            if not gt_file.exists():
                gt_file = self.test_data_dir / 'ground_truth.json'
            
            if not gt_file.exists():
                logger.info("No ground truth data available for comparison")
                return results
            
            # Load ground truth
            with open(gt_file, 'r') as f:
                if gt_file.suffix == '.yaml':
                    gt_data = yaml.safe_load(f)
                else:
                    gt_data = json.load(f)
            
            results['ground_truth_available'] = True
            
            # Compare camera matrix
            if 'camera_matrix' in gt_data:
                gt_matrix = np.array(gt_data['camera_matrix'])
                calib_matrix = self.calibration_data['camera_matrix']
                
                matrix_error = np.linalg.norm(gt_matrix - calib_matrix, 'fro')
                relative_error = matrix_error / np.linalg.norm(gt_matrix, 'fro')
                
                results['parameter_errors']['camera_matrix'] = {
                    'absolute_error': float(matrix_error),
                    'relative_error': float(relative_error)
                }
            
            # Compare distortion coefficients
            if 'distortion_coeffs' in gt_data:
                gt_dist = np.array(gt_data['distortion_coeffs'])
                calib_dist = self.calibration_data['distortion_coeffs']
                
                dist_error = np.linalg.norm(gt_dist - calib_dist)
                results['parameter_errors']['distortion_coeffs'] = {
                    'absolute_error': float(dist_error)
                }
            
            # Calculate overall error
            if results['parameter_errors']:
                all_errors = [v['absolute_error'] for v in results['parameter_errors'].values()]
                results['overall_error'] = float(np.mean(all_errors))
                
                # Quality assessment
                if results['overall_error'] > 0.1:
                    results['status'] = 'FAILED'
                    results['quality'] = 'POOR'
                elif results['overall_error'] > 0.05:
                    results['status'] = 'WARNING'
                    results['quality'] = 'FAIR'
                else:
                    results['status'] = 'PASSED'
                    results['quality'] = 'GOOD'
            
            logger.info(f"Ground truth comparison: overall error = {results['overall_error']:.6f}")
            
        except Exception as e:
            results['status'] = 'FAILED'
            results['error'] = str(e)
            logger.error(f"Ground truth comparison failed: {e}")
        
        return results
    
    def test_statistical_analysis(self) -> Dict:
        """Statistical analysis of calibration results."""
        logger.info("Performing statistical analysis...")
        
        results = {
            'status': 'PASSED',
            'normality_test': {},
            'confidence_intervals': {},
            'statistical_power': 0.0
        }
        
        try:
            # Collect reprojection errors
            all_errors = []
            test_frames = self.get_test_frames()
            
            for frame_data in test_frames:
                frame_error = self.compute_frame_reprojection_error(frame_data)
                if frame_error is not None:
                    all_errors.extend(frame_error)
            
            if len(all_errors) < 30:
                results['status'] = 'WARNING'
                results['warning'] = f'Insufficient samples for statistical analysis: {len(all_errors)}'
                return results
            
            # Normality test (Shapiro-Wilk)
            shapiro_stat, shapiro_p = stats.shapiro(all_errors[:5000])  # Limit to 5000 samples
            results['normality_test'] = {
                'shapiro_statistic': float(shapiro_stat),
                'shapiro_p_value': float(shapiro_p),
                'is_normal': shapiro_p > 0.05
            }
            
            # Confidence intervals
            confidence_level = 0.95
            alpha = 1 - confidence_level
            
            mean_error = np.mean(all_errors)
            std_error = np.std(all_errors, ddof=1)
            n = len(all_errors)
            
            # t-critical value
            t_critical = stats.t.ppf(1 - alpha/2, n-1)
            margin_error = t_critical * std_error / np.sqrt(n)
            
            results['confidence_intervals'] = {
                'confidence_level': confidence_level,
                'mean_lower': float(mean_error - margin_error),
                'mean_upper': float(mean_error + margin_error),
                'std_lower': float(std_error * np.sqrt((n-1) / stats.chi2.ppf(1 - alpha/2, n-1))),
                'std_upper': float(std_error * np.sqrt((n-1) / stats.chi2.ppf(alpha/2, n-1)))
            }
            
            # Statistical power (simplified)
            effect_size = mean_error / std_error if std_error > 0 else 0
            results['statistical_power'] = float(min(1.0, effect_size * 2))  # Simplified power calculation
            
            # Quality assessment
            if results['normality_test']['is_normal']:
                results['distribution'] = 'NORMAL'
            else:
                results['distribution'] = 'NON_NORMAL'
            
            logger.info(f"Statistical analysis: mean = {mean_error:.3f}, 95% CI = [{results['confidence_intervals']['mean_lower']:.3f}, {results['confidence_intervals']['mean_upper']:.3f}]")
            
        except Exception as e:
            results['status'] = 'FAILED'
            results['error'] = str(e)
            logger.error(f"Statistical analysis failed: {e}")
        
        return results
    
    def get_test_frames(self) -> List[Dict]:
        """Get test frames for evaluation."""
        frames = []
        
        # Look for extracted frames
        data_dir = self.test_data_dir / 'mav0' / 'cam0' / 'data'
        if not data_dir.exists():
            data_dir = self.test_data_dir / 'cam_april' / 'mav0' / 'cam0' / 'data'
        if not data_dir.exists():
            data_dir = self.test_data_dir / 'cam_checkerboard' / 'mav0' / 'cam0' / 'data'
        
        if not data_dir.exists():
            logger.warning("No test frames found")
            return frames
        
        # Load configuration
        config_file = self.test_data_dir / 'april_6x6.yaml'
        if not config_file.exists():
            config_file = self.test_data_dir / 'checkerboard_7x6.yaml'
        
        config = {}
        if config_file.exists():
            with open(config_file, 'r') as f:
                config = yaml.safe_load(f)
        
        # Process frames
        image_files = list(data_dir.glob('*.png'))[:50]  # Limit to 50 frames
        
        for img_file in image_files:
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
        
        logger.info(f"Loaded {len(frames)} test frames")
        return frames
    
    def compute_frame_reprojection_error(self, frame_data: Dict) -> Optional[List[float]]:
        """Compute reprojection error for a single frame."""
        try:
            img_points = frame_data['detected_corners']
            
            # Generate corresponding object points (simplified)
            num_points = len(img_points)
            objp = np.zeros((num_points, 3), dtype=np.float32)
            objp[:, :2] = np.mgrid[0:num_points:1].T.reshape(-1, 2) * 0.1  # 10cm spacing
            
            # Project object points
            img_points_proj, _ = cv2.projectPoints(objp, np.zeros(3), np.zeros(3), 
                                                   self.calibration_data['camera_matrix'], 
                                                   self.calibration_data['distortion_coeffs'])
            
            # Compute per-point errors
            errors = [cv2.norm(img_pts, proj_pt, cv2.NORM_L2) 
                     for img_pts, proj_pt in zip(img_points, img_points_proj.reshape(-1, 2))]
            
            return [float(e) for e in errors]
            
        except Exception as e:
            logger.error(f"Error computing frame reprojection error: {e}")
            return None
    
    def calibrate_on_frames(self, frames: List[Dict]) -> Dict:
        """Calibrate camera using given frames."""
        result = {'status': 'PASSED'}
        
        try:
            obj_points = []
            img_points = []
            
            for frame_data in frames:
                if 'detected_corners' not in frame_data:
                    continue
                
                img_pts = frame_data['detected_corners']
                num_points = len(img_pts)
                
                # Generate object points
                objp = np.zeros((num_points, 3), dtype=np.float32)
                objp[:, :2] = np.mgrid[0:num_points:1].T.reshape(-1, 2) * 0.1
                
                obj_points.append(objp)
                img_points.append(img_pts)
            
            if len(obj_points) < 5:
                result['status'] = 'FAILED'
                result['error'] = 'Insufficient calibration views'
                return result
            
            # Get image size
            img_shape = frames[0]['image'].shape[:2][::-1]
            
            # Calibrate
            ret, mtx, dist, rvecs, tvecs = cv2.calibrateCamera(
                obj_points, img_points, img_shape, None, None
            )
            
            if not ret:
                result['status'] = 'FAILED'
                result['error'] = 'Calibration failed'
                return result
            
            result['camera_matrix'] = mtx
            result['distortion_coeffs'] = dist
            result['reprojection_error'] = float(np.mean([cv2.norm(
                cv2.projectPoints(obj_points[i], rvecs[i], tvecs[i], mtx, dist)[0], 
                img_points[i], cv2.NORM_L2
            ) / len(img_points[i]) for i in range(len(obj_points))]))
            
        except Exception as e:
            result['status'] = 'FAILED'
            result['error'] = str(e)
        
        return result
    
    def validate_calibration(self, calib_result: Dict, test_frames: List[Dict]) -> float:
        """Validate calibration on test frames."""
        if calib_result['status'] != 'PASSED':
            return 0.0
        
        try:
            total_error = 0.0
            valid_frames = 0
            
            for frame_data in test_frames:
                if 'detected_corners' not in frame_data:
                    continue
                
                img_points = frame_data['detected_corners']
                num_points = len(img_points)
                
                # Generate object points
                objp = np.zeros((num_points, 3), dtype=np.float32)
                objp[:, :2] = np.mgrid[0:num_points:1].T.reshape(-1, 2) * 0.1
                
                # Project and compute error
                img_points_proj, _ = cv2.projectPoints(objp, np.zeros(3), np.zeros(3),
                                                       calib_result['camera_matrix'],
                                                       calib_result['distortion_coeffs'])
                
                error = cv2.norm(img_points, img_points_proj.reshape(-1, 2), cv2.NORM_L2) / len(img_points)
                total_error += error
                valid_frames += 1
            
            if valid_frames == 0:
                return 0.0
            
            mean_error = total_error / valid_frames
            # Convert error to score (lower error = higher score)
            score = max(0.0, 1.0 - mean_error / 5.0)  # 5 pixels = 0 score
            
            return float(score)
            
        except Exception as e:
            logger.error(f"Error in validation: {e}")
            return 0.0
    
    def plot_error_histogram(self, errors: List[float], filename: str):
        """Plot error histogram."""
        plt.figure(figsize=(10, 6))
        plt.hist(errors, bins=50, alpha=0.7, edgecolor='black')
        plt.xlabel('Reprojection Error (pixels)')
        plt.ylabel('Frequency')
        plt.title('Reprojection Error Distribution')
        plt.grid(True, alpha=0.3)
        plt.savefig(self.output_dir / filename, dpi=300, bbox_inches='tight')
        plt.close()
    
    def plot_cross_validation_results(self, fold_scores: List[float], filename: str):
        """Plot cross-validation results."""
        plt.figure(figsize=(10, 6))
        plt.plot(range(1, len(fold_scores) + 1), fold_scores, 'bo-', linewidth=2, markersize=8)
        plt.xlabel('Fold')
        plt.ylabel('Validation Score')
        plt.title('Cross-Validation Results')
        plt.ylim([0, 1])
        plt.grid(True, alpha=0.3)
        plt.savefig(self.output_dir / filename, dpi=300, bbox_inches='tight')
        plt.close()
    
    def plot_parameter_stability(self, parameter_variations: Dict, filename: str):
        """Plot parameter stability."""
        plt.figure(figsize=(12, 8))
        
        params = list(parameter_variations.keys())
        means = [parameter_variations[p]['mean'] for p in params]
        stds = [parameter_variations[p]['std'] for p in params]
        
        x_pos = np.arange(len(params))
        
        plt.bar(x_pos, means, yerr=stds, alpha=0.7, capsize=10)
        plt.xlabel('Parameters')
        plt.ylabel('Parameter Values')
        plt.title('Calibration Parameter Stability')
        plt.xticks(x_pos, params, rotation=45)
        plt.grid(True, alpha=0.3)
        plt.tight_layout()
        plt.savefig(self.output_dir / filename, dpi=300, bbox_inches='tight')
        plt.close()
    
    def generate_accuracy_report(self):
        """Generate comprehensive accuracy report."""
        report = {
            'summary': {
                'calibration_file': str(self.calibration_file),
                'test_data_dir': str(self.test_data_dir),
                'overall_status': 'PASSED',
                'tests_performed': len(self.test_results),
                'passed_tests': 0,
                'failed_tests': 0,
                'warning_tests': 0
            },
            'detailed_results': self.test_results,
            'recommendations': []
        }
        
        # Count test statuses
        for test_name, result in self.test_results.items():
            status = result.get('status', 'UNKNOWN')
            if status == 'PASSED':
                report['summary']['passed_tests'] += 1
            elif status == 'FAILED':
                report['summary']['failed_tests'] += 1
                report['summary']['overall_status'] = 'FAILED'
            elif status == 'WARNING':
                report['summary']['warning_tests'] += 1
                if report['summary']['overall_status'] != 'FAILED':
                    report['summary']['overall_status'] = 'WARNING'
        
        # Generate recommendations
        if self.test_results.get('reprojection_error_analysis', {}).get('status') == 'FAILED':
            report['recommendations'].append("Reprojection error too high - consider recalibrating with better data")
        
        if self.test_results.get('cross_validation', {}).get('status') == 'FAILED':
            report['recommendations'].append("Cross-validation failed - calibration may be overfitting")
        
        if self.test_results.get('calibration_stability', {}).get('status') == 'FAILED':
            report['recommendations'].append("Calibration unstable - collect more diverse calibration data")
        
        # Save report
        report_file = self.output_dir / 'calibration_accuracy_report.json'
        with open(report_file, 'w') as f:
            json.dump(report, f, indent=2)
        
        logger.info(f"Accuracy report saved to: {report_file}")

def main():
    parser = argparse.ArgumentParser(description='Test calibration accuracy')
    parser.add_argument('--calibration-results', required=True, help='Calibration results file')
    parser.add_argument('--test-data', required=True, help='Test data directory')
    parser.add_argument('--output-dir', default='accuracy_results', help='Output directory')
    
    args = parser.parse_args()
    
    tester = CalibrationAccuracyTester(args.calibration_results, args.test_data, args.output_dir)
    results = tester.run_comprehensive_tests()
    
    # Print summary
    print("\n" + "="*60)
    print("CALIBRATION ACCURACY TEST SUMMARY")
    print("="*60)
    
    for test_name, result in results.items():
        status = result.get('status', 'UNKNOWN')
        status_icon = "✅" if status == 'PASSED' else "⚠️" if status == 'WARNING' else "❌" if status == 'FAILED' else "⏭️"
        print(f"{status_icon} {test_name}: {status}")
    
    print("="*60)

if __name__ == '__main__':
    main()
