<?php
$conn = mysqli_connect("localhost", "iot", "pwiot");
mysqli_select_db($conn, "iotdb");

// 최신 데이터 조회
$result = mysqli_query($conn, "SELECT * FROM sensor ORDER BY date DESC, time DESC LIMIT 15");
$data = array();
while($row = mysqli_fetch_assoc($result)) {
    $data[] = $row;
}

// 오늘 통계 데이터
$stats_query = "SELECT 
    AVG(temp) as avg_temp, 
    AVG(humi) as avg_humi, 
    AVG(ptcl) as avg_ptcl,
    COUNT(*) as total_records,
    MAX(temp) as max_temp,
    MIN(temp) as min_temp,
    MAX(humi) as max_humi,
    MIN(humi) as min_humi
    FROM sensor 
    WHERE date = CURDATE()";
$stats_result = mysqli_query($conn, $stats_query);
$stats = mysqli_fetch_assoc($stats_result);

mysqli_close($conn);
?>
<!DOCTYPE html>
<html lang="ko">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>센서 테이블</title>
    <style>
        :root {
            --primary: #9f7aea;
            --success: #38a169;
            --warning: #ed8936;
            --error: #e53e3e;
            --bg: #f7fafc;
            --card: #ffffff;
            --text: #2d3748;
            --text-light: #718096;
            --border: #e2e8f0;
        }

        * { margin: 0; padding: 0; box-sizing: border-box; }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: var(--bg);
            color: var(--text);
            padding: 20px;
            line-height: 1.6;
        }

        .container {
            max-width: 100%;
            margin: 0 auto;
        }

        .stats-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
            margin-bottom: 25px;
        }

        .stat-card {
            background: var(--card);
            border-radius: 12px;
            padding: 20px;
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.05);
            border: 1px solid var(--border);
            text-align: center;
        }

        .stat-label {
            color: var(--text-light);
            font-size: 12px;
            text-transform: uppercase;
            letter-spacing: 1px;
            margin-bottom: 8px;
        }

        .stat-value {
            font-size: 24px;
            font-weight: 700;
            color: var(--primary);
        }

        .stat-unit {
            font-size: 14px;
            color: var(--text-light);
            margin-left: 4px;
        }

        .table-container {
            background: var(--card);
            border-radius: 12px;
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.05);
            border: 1px solid var(--border);
            overflow: hidden;
        }

        .table-header {
            background: linear-gradient(135deg, var(--primary), #667eea);
            color: white;
            padding: 20px;
            text-align: center;
        }

        .table-title {
            font-size: 18px;
            font-weight: 600;
            margin-bottom: 5px;
        }

        .table-subtitle {
            opacity: 0.9;
            font-size: 14px;
        }

        table {
            width: 100%;
            border-collapse: collapse;
        }

        th, td {
            padding: 12px 15px;
            text-align: left;
            border-bottom: 1px solid var(--border);
        }

        th {
            background: var(--bg);
            font-weight: 600;
            color: var(--text);
            font-size: 14px;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }

        td {
            font-size: 14px;
        }

        tr:hover {
            background: rgba(159, 122, 234, 0.05);
        }

        .sensor-name {
            font-weight: 600;
            color: var(--primary);
        }

        .temp {
            color: var(--error);
            font-weight: 600;
        }

        .humi {
            color: #3182ce;
            font-weight: 600;
        }

        .ptcl {
            color: var(--warning);
            font-weight: 600;
        }

        .time {
            color: var(--text-light);
            font-size: 13px;
        }

        .update-time {
            text-align: center;
            padding: 15px;
            color: var(--text-light);
            font-size: 12px;
            background: var(--bg);
        }

        .no-data {
            text-align: center;
            padding: 40px;
            color: var(--text-light);
        }
    </style>
</head>
<body>
    <div class="container">
        <!-- 통계 카드 -->
        <div class="stats-grid">
            <div class="stat-card">
                <div class="stat-label">평균 온도</div>
                <div class="stat-value">
                    <?= $stats['avg_temp'] ? number_format($stats['avg_temp'], 1) : '--' ?>
                    <span class="stat-unit">°C</span>
                </div>
            </div>
            <div class="stat-card">
                <div class="stat-label">평균 습도</div>
                <div class="stat-value">
                    <?= $stats['avg_humi'] ? number_format($stats['avg_humi'], 1) : '--' ?>
                    <span class="stat-unit">%</span>
                </div>
            </div>
            <div class="stat-card">
                <div class="stat-label">평균 파티클</div>
                <div class="stat-value">
                    <?= $stats['avg_ptcl'] ? number_format($stats['avg_ptcl'], 0) : '--' ?>
                    <span class="stat-unit">ug</span>
                </div>
            </div>
            <div class="stat-card">
                <div class="stat-label">오늘 데이터</div>
                <div class="stat-value">
                    <?= $stats['total_records'] ?: '0' ?>
                    <span class="stat-unit">건</span>
                </div>
            </div>
        </div>

        <!-- 센서 테이블 -->
        <div class="table-container">
            <div class="table-header">
                <div class="table-title">최신 센서 데이터</div>
                <div class="table-subtitle">실시간 모니터링 (최근 15건)</div>
            </div>

            <?php if (count($data) > 0): ?>
                <table>
                    <thead>
                        <tr>
                            <th>센서명</th>
                            <th>온도</th>
                            <th>습도</th>
                            <th>파티클</th>
                            <th>날짜</th>
                            <th>시간</th>
                        </tr>
                    </thead>
                    <tbody>
                        <?php foreach($data as $row): ?>
                            <tr>
                                <td class="sensor-name"><?= htmlspecialchars($row['name']) ?></td>
                                <td class="temp"><?= number_format($row['temp'], 1) ?>°C</td>
                                <td class="humi"><?= number_format($row['humi'], 1) ?>%</td>
                                <td class="ptcl"><?= number_format($row['ptcl'], 0) ?>ug</td>
                                <td><?= $row['date'] ?></td>
                                <td class="time"><?= $row['time'] ?></td>
                            </tr>
                        <?php endforeach; ?>
                    </tbody>
                </table>
            <?php else: ?>
                <div class="no-data">
                    <h3>데이터가 없습니다</h3>
                    <p>센서 데이터를 확인해주세요.</p>
                </div>
            <?php endif; ?>

            <div class="update-time">
                마지막 업데이트: <?= date('Y-m-d H:i:s') ?>
            </div>
        </div>
    </div>

    <script>
        // 자동 새로고침 (부모 페이지에서 제어)
        window.addEventListener('focus', function() {
            // 포커스가 돌아오면 새로고침
            setTimeout(() => {
                location.reload();
            }, 100);
        });
    </script>
</body>
</html>
