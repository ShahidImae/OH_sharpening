import cv2
import numpy as np
import os, csv


def tenengrad(img_path):
    """
    计算图片的tenengrad分数，以衡量图片清晰度，越清晰数值越大
    """
    img = cv2.imread(img_path, cv2.IMREAD_GRAYSCALE)

    gradient_x = cv2.Sobel(img, cv2.CV_64F, 1, 0, ksize=3)
    gradient_y = cv2.Sobel(img, cv2.CV_64F, 0, 1, ksize=3)

    gradient_magnitude = np.sqrt(gradient_x ** 2 + gradient_y ** 2)

    tenengrad_score = np.sum(gradient_magnitude ** 2)
    return tenengrad_score


if __name__ == '__main__':
    dir1_path = '.\\data\\raw\\'
    dir2_path = '.\\data\\processed\\'

    dir1_scores = []
    dir2_scores = []

    # 遍历第一个目录
    print(f"正在处理目录: {dir1_path}")
    for filename in os.listdir(dir1_path):
        if filename.lower().endswith(".jpg"):
            img_path = os.path.join(dir1_path, filename)
            try:
                score = tenengrad(img_path)
                dir1_scores.append(score)
                print(f"  {filename}: {score:.2f}")
            except Exception as e:
                print(f"处理 {filename} 时发生错误: {e}")

    # 遍历第二个目录
    print(f"正在处理目录: {dir2_path}")
    for filename in os.listdir(dir2_path):
        if filename.lower().endswith(".jpg"):
            img_path = os.path.join(dir2_path, filename)
            try:
                score = tenengrad(img_path)
                dir2_scores.append(score)
                print(f" {filename}: {score:.2f}")
            except Exception as e:
                print(f"处理 {filename} 时发生错误: {e}")

    # 将得分写入 CSV 文件
    csv_file = "clarity_scores.csv"
    with open(csv_file, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['Directory 1 Scores', 'Directory 2 Scores'])  # 写入表头

        max_len = max(len(dir1_scores), len(dir2_scores))
        for i in range(max_len):
            row = [dir1_scores[i] if i < len(dir1_scores) else '',
                   dir2_scores[i] if i < len(dir2_scores) else '']
            writer.writerow(row)

    print(f"图片清晰度得分已保存到: {csv_file}")
    # 计算并打印平均值
    avg_dir1 = np.mean(dir1_scores) if dir1_scores else 0
    avg_dir2 = np.mean(dir2_scores) if dir2_scores else 0

    print(f"未处理图片的平均清晰度得分: {avg_dir1:.2f}")
    print(f"处理后图片的平均清晰度得分: {avg_dir2:.2f}")
