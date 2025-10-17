package aws

import (
	"bytes"
	"context"
	"fmt"
	"time"

	"github.com/aws/aws-sdk-go-v2/aws"
	"github.com/aws/aws-sdk-go-v2/config"
	"github.com/aws/aws-sdk-go-v2/service/s3"
)

type S3Client struct {
	client *s3.Client
	bucket string
}

// NewS3Client creates a new S3 client using the kaimz_tdr AWS profile
func NewS3Client(ctx context.Context, region, bucket string) (*S3Client, error) {
	cfg, err := config.LoadDefaultConfig(ctx, 
		config.WithSharedConfigProfile("kaimz_tdr"),
		config.WithRegion(region),
	)
	if err != nil {
		return nil, fmt.Errorf("unable to load SDK config: %w", err)
	}

	return &S3Client{
		client: s3.NewFromConfig(cfg),
		bucket: bucket,
	}, nil
}

// UploadLog uploads a log string to S3 with a timestamped filename
func (s *S3Client) UploadLog(ctx context.Context, logContent string) (string, error) {
	// Generate timestamped filename
	timestamp := time.Now().Format("2006-01-02_15-04-05")
	key := fmt.Sprintf("logs/%s.log", timestamp)

	// Upload to S3
	_, err := s.client.PutObject(ctx, &s3.PutObjectInput{
		Bucket:      aws.String(s.bucket),
		Key:         aws.String(key),
		Body:        bytes.NewReader([]byte(logContent)),
		ContentType: aws.String("text/plain"),
	})

	if err != nil {
		return "", fmt.Errorf("failed to upload log to S3: %w", err)
	}

	// Return the S3 URL
	url := fmt.Sprintf("s3://%s/%s", s.bucket, key)
	return url, nil
}

// UploadLogWithKey uploads a log string to S3 with a custom key/filename
func (s *S3Client) UploadLogWithKey(ctx context.Context, key string, logContent string) (string, error) {
	_, err := s.client.PutObject(ctx, &s3.PutObjectInput{
		Bucket:      aws.String(s.bucket),
		Key:         aws.String(key),
		Body:        bytes.NewReader([]byte(logContent)),
		ContentType: aws.String("text/plain"),
	})

	if err != nil {
		return "", fmt.Errorf("failed to upload log to S3: %w", err)
	}

	url := fmt.Sprintf("s3://%s/%s", s.bucket, key)
	return url, nil
}

// ListLogs lists all log files in the logs/ prefix
func (s *S3Client) ListLogs(ctx context.Context) ([]string, error) {
	result, err := s.client.ListObjectsV2(ctx, &s3.ListObjectsV2Input{
		Bucket: aws.String(s.bucket),
		Prefix: aws.String("logs/"),
	})

	if err != nil {
		return nil, fmt.Errorf("failed to list logs from S3: %w", err)
	}

	var keys []string
	for _, obj := range result.Contents {
		keys = append(keys, *obj.Key)
	}

	return keys, nil
}