#!/usr/bin/env python3
"""
ROSBAG Processing Utilities

This script provides utilities for processing ROSBAG files containing
calibration data, including frame extraction, topic analysis, and data
conversion to CSV format.

Usage:
    python rosbag_processor.py --input BAG_FILE --output OUTPUT_DIR [OPTIONS]
"""

import os
import sys
import cv2
import numpy as np
import yaml
import json
import argparse
import csv
import struct
from pathlib import Path
from typing import Dict, List, Tuple, Optional, Any
import logging
from datetime import datetime

# Try to import rosbag
try:
    import rosbag
    ROSBAG_AVAILABLE = True
except ImportError:
    ROSBAG_AVAILABLE = False
    logging.warning("rosbag package not available. Limited functionality.")

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class ROSBagProcessor:
    """Process ROSBAG files for calibration data extraction."""
    
    def __init__(self, bag_file: str, output_dir: str):
        self.bag_file = Path(bag_file)
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(exist_ok=True)
        
        self.bag = None
        self.topics = {}
        self.extracted_frames = []
        
        if ROSBAG_AVAILABLE:
            try:
                self.bag = rosbag.Bag(str(self.bag_file))
                logger.info(f"Opened ROSBAG: {self.bag_file}")
            except Exception as e:
                logger.error(f"Failed to open ROSBAG: {e}")
                raise
        else:
            logger.warning("ROSBAG processing not available without rosbag package")
    
    def __enter__(self):
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.bag:
            self.bag.close()
    
    def analyze_bag(self) -> Dict:
        """Analyze ROSBAG structure and topics."""
        if not self.bag:
            logger.error("ROSBAG not available for analysis")
            return {}
        
        logger.info("Analyzing ROSBAG structure...")
        
        analysis = {
            'bag_file': str(self.bag_file),
            'start_time': None,
            'end_time': None,
            'duration': None,
            'topics': {},
            'message_counts': {},
            'total_messages': 0,
            'size_mb': self.bag_file.stat().st_size / (1024 * 1024)
        }
        
        # Get bag info
        bag_info = self.bag.get_type_and_topic_info()
        
        # Extract topic information
        for topic_name, topic_info in bag_info.topics.items():
            analysis['topics'][topic_name] = {
                'type': topic_info.msg_type,
                'message_count': topic_info.message_count,
                'connections': topic_info.connections
            }
            analysis['message_counts'][topic_name] = topic_info.message_count
            analysis['total_messages'] += topic_info.message_count
        
        # Get time information
        if self.bag.get_message_count() > 0:
            start_time = self.bag.get_start_time()
            end_time = self.bag.get_end_time()
            
            analysis['start_time'] = datetime.fromtimestamp(start_time).isoformat()
            analysis['end_time'] = datetime.fromtimestamp(end_time).isoformat()
            analysis['duration'] = end_time - start_time
        
        # Save analysis
        analysis_file = self.output_dir / 'bag_analysis.json'
        with open(analysis_file, 'w') as f:
            json.dump(analysis, f, indent=2)
        
        logger.info(f"Bag analysis saved to: {analysis_file}")
        return analysis
    
    def extract_image_frames(self, image_topics: List[str], max_frames: Optional[int] = None) -> Dict:
        """Extract image frames from specified topics."""
        if not self.bag:
            logger.error("ROSBAG not available for frame extraction")
            return {}
        
        logger.info(f"Extracting image frames from topics: {image_topics}")
        
        extracted_data = {}
        
        for topic in image_topics:
            if topic not in self.bag.get_type_and_topic_info().topics:
                logger.warning(f"Topic {topic} not found in bag")
                continue
            
            topic_dir = self.output_dir / topic.replace('/', '_')
            topic_dir.mkdir(exist_ok=True)
            
            frames_data = []
            frame_count = 0
            
            for topic_name, msg, timestamp in self.bag.read_messages(topics=[topic]):
                if max_frames and frame_count >= max_frames:
                    break
                
                try:
                    # Convert ROS image to OpenCV format
                    cv_image = self.ros_image_to_cv2(msg)
                    if cv_image is None:
                        continue
                    
                    # Save frame
                    frame_filename = f"{int(timestamp.to_nsec() / 1000)}.png"
                    frame_path = topic_dir / frame_filename
                    
                    cv2.imwrite(str(frame_path), cv_image)
                    
                    # Record frame info
                    frame_info = {
                        'timestamp': timestamp.to_nsec(),
                        'filename': frame_filename,
                        'width': cv_image.shape[1],
                        'height': cv_image.shape[0],
                        'channels': cv_image.shape[2] if len(cv_image.shape) > 2 else 1
                    }
                    frames_data.append(frame_info)
                    
                    frame_count += 1
                    
                    if frame_count % 100 == 0:
                        logger.info(f"Extracted {frame_count} frames from {topic}")
                
                except Exception as e:
                    logger.error(f"Error processing frame from {topic}: {e}")
                    continue
            
            # Save frame metadata
            metadata_file = topic_dir / 'frames_metadata.json'
            with open(metadata_file, 'w') as f:
                json.dump(frames_data, f, indent=2)
            
            extracted_data[topic] = {
                'frames_count': frame_count,
                'topic_dir': str(topic_dir),
                'metadata_file': str(metadata_file)
            }
            
            logger.info(f"Extracted {frame_count} frames from {topic}")
        
        return extracted_data
    
    def extract_sensor_data(self, sensor_topics: List[str]) -> Dict:
        """Extract sensor data to CSV format."""
        if not self.bag:
            logger.error("ROSBAG not available for sensor data extraction")
            return {}
        
        logger.info(f"Extracting sensor data from topics: {sensor_topics}")
        
        extracted_data = {}
        
        for topic in sensor_topics:
            if topic not in self.bag.get_type_and_topic_info().topics:
                logger.warning(f"Topic {topic} not found in bag")
                continue
            
            topic_dir = self.output_dir / topic.replace('/', '_')
            topic_dir.mkdir(exist_ok=True)
            
            csv_file = topic_dir / 'sensor_data.csv'
            
            with open(csv_file, 'w', newline='') as csvfile:
                writer = csv.writer(csvfile)
                
                # Write header
                writer.writerow(['timestamp', 'seq', 'frame_id', 'data'])
                
                message_count = 0
                
                for topic_name, msg, timestamp in self.bag.read_messages(topics=[topic]):
                    try:
                        # Extract message data
                        msg_data = self.extract_message_data(msg)
                        
                        # Write to CSV
                        writer.writerow([
                            timestamp.to_nsec(),
                            getattr(msg, 'header', {}).get('seq', ''),
                            getattr(msg, 'header', {}).get('frame_id', ''),
                            json.dumps(msg_data)
                        ])
                        
                        message_count += 1
                        
                        if message_count % 1000 == 0:
                            logger.info(f"Processed {message_count} messages from {topic}")
                    
                    except Exception as e:
                        logger.error(f"Error processing message from {topic}: {e}")
                        continue
            
            extracted_data[topic] = {
                'messages_count': message_count,
                'csv_file': str(csv_file)
            }
            
            logger.info(f"Extracted {message_count} messages from {topic}")
        
        return extracted_data
    
    def extract_imu_data(self, imu_topics: List[str]) -> Dict:
        """Extract IMU data with specific formatting."""
        if not self.bag:
            logger.error("ROSBAG not available for IMU data extraction")
            return {}
        
        logger.info(f"Extracting IMU data from topics: {imu_topics}")
        
        extracted_data = {}
        
        for topic in imu_topics:
            if topic not in self.bag.get_type_and_topic_info().topics:
                logger.warning(f"Topic {topic} not found in bag")
                continue
            
            topic_dir = self.output_dir / topic.replace('/', '_')
            topic_dir.mkdir(exist_ok=True)
            
            csv_file = topic_dir / 'imu_data.csv'
            
            with open(csv_file, 'w', newline='') as csvfile:
                writer = csv.writer(csvfile)
                
                # Write header
                writer.writerow([
                    'timestamp', 'seq', 'frame_id',
                    'linear_accel_x', 'linear_accel_y', 'linear_accel_z',
                    'angular_vel_x', 'angular_vel_y', 'angular_vel_z',
                    'orientation_x', 'orientation_y', 'orientation_z', 'orientation_w'
                ])
                
                message_count = 0
                
                for topic_name, msg, timestamp in self.bag.read_messages(topics=[topic]):
                    try:
                        # Extract IMU data
                        writer.writerow([
                            timestamp.to_nsec(),
                            msg.header.seq,
                            msg.header.frame_id,
                            msg.linear_acceleration.x,
                            msg.linear_acceleration.y,
                            msg.linear_acceleration.z,
                            msg.angular_velocity.x,
                            msg.angular_velocity.y,
                            msg.angular_velocity.z,
                            msg.orientation.x,
                            msg.orientation.y,
                            msg.orientation.z,
                            msg.orientation.w
                        ])
                        
                        message_count += 1
                        
                        if message_count % 1000 == 0:
                            logger.info(f"Processed {message_count} IMU messages from {topic}")
                    
                    except AttributeError as e:
                        logger.warning(f"Skipping message with missing fields from {topic}: {e}")
                        continue
                    except Exception as e:
                        logger.error(f"Error processing IMU message from {topic}: {e}")
                        continue
            
            extracted_data[topic] = {
                'messages_count': message_count,
                'csv_file': str(csv_file)
            }
            
            logger.info(f"Extracted {message_count} IMU messages from {topic}")
        
        return extracted_data
    
    def generate_calibration_dataset(self, config: Dict) -> bool:
        """Generate a complete calibration dataset structure."""
        logger.info("Generating calibration dataset structure...")
        
        try:
            # Create output structure
            dataset_name = config.get('dataset_name', 'calibration_dataset')
            dataset_dir = self.output_dir / dataset_name
            dataset_dir.mkdir(exist_ok=True)
            
            # Create main configuration
            config_file = dataset_dir / 'calibration_config.yaml'
            with open(config_file, 'w') as f:
                yaml.dump(config, f, default_flow_style=False)
            
            # Create extracted data directory (e.g., mav0)
            extracted_dir = dataset_dir / 'mav0'
            extracted_dir.mkdir(exist_ok=True)
            
            # Process image topics
            image_topics = config.get('image_topics', [])
            if image_topics:
                image_data = self.extract_image_frames(image_topics)
                
                # Organize images into cam0, cam1 structure
                for i, topic in enumerate(image_topics):
                    cam_dir = extracted_dir / f'cam{i}'
                    cam_dir.mkdir(exist_ok=True)
                    
                    # Move extracted frames
                    topic_dir = self.output_dir / topic.replace('/', '_')
                    if topic_dir.exists():
                        data_dir = cam_dir / 'data'
                        data_dir.mkdir(exist_ok=True)
                        
                        # Move images
                        for img_file in topic_dir.glob('*.png'):
                            img_file.rename(data_dir / img_file.name)
                        
                        # Move metadata
                        metadata_file = topic_dir / 'frames_metadata.json'
                        if metadata_file.exists():
                            metadata_file.rename(cam_dir / 'data.csv')
                        
                        # Create sensor configuration
                        sensor_config = self.create_sensor_config(topic, i, config)
                        sensor_yaml = cam_dir / 'sensor.yaml'
                        with open(sensor_yaml, 'w') as f:
                            yaml.dump(sensor_config, f, default_flow_style=False)
            
            # Process IMU topics
            imu_topics = config.get('imu_topics', [])
            if imu_topics:
                imu_data = self.extract_imu_data(imu_topics)
                
                for i, topic in enumerate(imu_topics):
                    imu_dir = extracted_dir / f'imu{i}'
                    imu_dir.mkdir(exist_ok=True)
                    
                    # Move CSV data
                    topic_dir = self.output_dir / topic.replace('/', '_')
                    csv_file = topic_dir / 'imu_data.csv'
                    if csv_file.exists():
                        csv_file.rename(imu_dir / 'data.csv')
                    
                    # Create sensor configuration
                    sensor_config = self.create_imu_config(topic, i, config)
                    sensor_yaml = imu_dir / 'sensor.yaml'
                    with open(sensor_yaml, 'w') as f:
                        yaml.dump(sensor_config, f, default_flow_style=False)
            
            logger.info(f"Calibration dataset generated: {dataset_dir}")
            return True
            
        except Exception as e:
            logger.error(f"Failed to generate calibration dataset: {e}")
            return False
    
    def ros_image_to_cv2(self, msg) -> Optional[np.ndarray]:
        """Convert ROS image message to OpenCV format."""
        try:
            # Handle different ROS image types
            if hasattr(msg, 'encoding'):
                encoding = msg.encoding
            else:
                encoding = 'bgr8'  # Default
            
            # Convert based on encoding
            if encoding == 'bgr8':
                return np.frombuffer(msg.data, dtype=np.uint8).reshape(msg.height, msg.width, 3)
            elif encoding == 'rgb8':
                rgb = np.frombuffer(msg.data, dtype=np.uint8).reshape(msg.height, msg.width, 3)
                return cv2.cvtColor(rgb, cv2.COLOR_RGB2BGR)
            elif encoding == 'mono8':
                return np.frombuffer(msg.data, dtype=np.uint8).reshape(msg.height, msg.width)
            elif encoding == 'mono16':
                return np.frombuffer(msg.data, dtype=np.uint16).reshape(msg.height, msg.width)
            else:
                logger.warning(f"Unsupported image encoding: {encoding}")
                return None
                
        except Exception as e:
            logger.error(f"Error converting ROS image: {e}")
            return None
    
    def extract_message_data(self, msg) -> Dict:
        """Extract relevant data from ROS message."""
        data = {}
        
        # Try to extract common fields
        for attr_name in dir(msg):
            if not attr_name.startswith('_'):
                try:
                    attr_value = getattr(msg, attr_name)
                    if not callable(attr_value):
                        # Handle different data types
                        if hasattr(attr_value, '__dict__'):
                            # Nested object
                            data[attr_name] = str(attr_value)
                        elif isinstance(attr_value, (list, tuple)):
                            # Array-like
                            data[attr_name] = list(attr_value)
                        else:
                            # Simple value
                            data[attr_name] = attr_value
                except Exception:
                    continue
        
        return data
    
    def create_sensor_config(self, topic: str, cam_id: int, config: Dict) -> Dict:
        """Create sensor configuration for camera."""
        sensor_config = {
            'sensor_type': 'camera',
            'comment': f'Camera {cam_id} from topic {topic}',
            'T_BS': {
                'cols': 4,
                'rows': 4,
                'data': config.get('camera_extrinsics', {}).get(f'cam{cam_id}', [
                    1.0, 0.0, 0.0, 0.0,
                    0.0, 1.0, 0.0, 0.0,
                    0.0, 0.0, 1.0, 0.0,
                    0.0, 0.0, 0.0, 1.0
                ])
            },
            'rate_hz': config.get('camera_rate', 20),
            'resolution': config.get('camera_resolution', [752, 480])
        }
        
        return sensor_config
    
    def create_imu_config(self, topic: str, imu_id: int, config: Dict) -> Dict:
        """Create sensor configuration for IMU."""
        sensor_config = {
            'sensor_type': 'imu',
            'comment': f'IMU {imu_id} from topic {topic}',
            'T_BS': {
                'cols': 4,
                'rows': 4,
                'data': config.get('imu_extrinsics', {}).get(f'imu{imu_id}', [
                    1.0, 0.0, 0.0, 0.0,
                    0.0, 1.0, 0.0, 0.0,
                    0.0, 0.0, 1.0, 0.0,
                    0.0, 0.0, 0.0, 1.0
                ])
            },
            'rate_hz': config.get('imu_rate', 200),
            'gravity_magnitude': 9.81
        }
        
        return sensor_config

def create_sample_config() -> Dict:
    """Create sample configuration for dataset generation."""
    return {
        'dataset_name': 'extracted_calibration',
        'target_type': 'aprilgrid',
        'tagCols': 6,
        'tagRows': 6,
        'tagSize': 0.088,
        'tagSpacing': 0.3,
        'image_topics': ['/cam0/image_raw', '/cam1/image_raw'],
        'imu_topics': ['/imu0/data'],
        'camera_rate': 20,
        'camera_resolution': [752, 480],
        'imu_rate': 200
    }

def main():
    parser = argparse.ArgumentParser(description='Process ROSBAG files for calibration')
    parser.add_argument('--input', '-i', required=True, help='Input ROSBAG file')
    parser.add_argument('--output', '-o', required=True, help='Output directory')
    parser.add_argument('--action', choices=['analyze', 'extract_images', 'extract_sensor', 'extract_imu', 'generate_dataset'], 
                       default='analyze', help='Action to perform')
    parser.add_argument('--topics', nargs='+', help='Topics to process')
    parser.add_argument('--max-frames', type=int, help='Maximum frames to extract')
    parser.add_argument('--config', help='Configuration file for dataset generation')
    
    args = parser.parse_args()
    
    if not ROSBAG_AVAILABLE and args.action != 'analyze':
        logger.error("rosbag package required for this action")
        sys.exit(1)
    
    with ROSBagProcessor(args.input, args.output) as processor:
        if args.action == 'analyze':
            analysis = processor.analyze_bag()
            print(f"Analysis complete. Total topics: {len(analysis.get('topics', {}))}")
        
        elif args.action == 'extract_images':
            if not args.topics:
                logger.error("Image topics required for extraction")
                sys.exit(1)
            
            image_data = processor.extract_image_frames(args.topics, args.max_frames)
            print(f"Extracted images from {len(image_data)} topics")
        
        elif args.action == 'extract_sensor':
            if not args.topics:
                logger.error("Sensor topics required for extraction")
                sys.exit(1)
            
            sensor_data = processor.extract_sensor_data(args.topics)
            print(f"Extracted sensor data from {len(sensor_data)} topics")
        
        elif args.action == 'extract_imu':
            if not args.topics:
                logger.error("IMU topics required for extraction")
                sys.exit(1)
            
            imu_data = processor.extract_imu_data(args.topics)
            print(f"Extracted IMU data from {len(imu_data)} topics")
        
        elif args.action == 'generate_dataset':
            if args.config:
                with open(args.config, 'r') as f:
                    config = yaml.safe_load(f)
            else:
                config = create_sample_config()
            
            success = processor.generate_calibration_dataset(config)
            if success:
                print("Calibration dataset generated successfully")
            else:
                print("Failed to generate calibration dataset")
                sys.exit(1)

if __name__ == '__main__':
    main()
