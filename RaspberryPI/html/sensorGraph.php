<?php
$conn = mysqli_connect("localhost", "iot", "pwiot");
mysqli_select_db($conn, "iotdb");

// 시간별 데이터 (최근 24시간)
$time_query = "SELECT 
    DATE_FORMAT(CONCAT(date, ' ', time), '%H:%i') as time_label,
    AVG(temp) as avg_temp,
    AVG(humi) as avg_humi,
    AVG(ptcl) as avg_ptcl
    FROM sensor 
    WHERE date >= CURDATE() - INTERVAL 1 DAY
    GROUP BY HOUR(time), DATE(date)
    ORDER BY date, time
    LIMIT 24";

$time_result = mysqli_query($conn, $time_query);
$time_data = array();
while($row = mysqli_fetch_assoc($time_result)) {
    $time_data[] = $row;
}

// 센서별 최신 데이터 비교
$sensor_query = "SELECT 
    name,
    temp,
    humi,
    ptcl,
    date,
    time
    FROM sensor s1
    WHERE (date, time) = (
        SELECT date, time 
        FROM sensor s2 
        WHERE s2.name = s1.name 
        ORDER BY date DESC, time DESC 
        LIMIT 1
    )
    ORDER BY name";

$sensor_result = mysqli_query($conn, $sensor_query);
$sensor_data = array();
while($row = mysqli_fetch_assoc($sensor_result)) {
    $sensor_data[] = $row;
}

mysqli_close($conn);
?>
<!DOCTYPE html>
<html lang="ko">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>센서 그래프</title>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/3.9.1/chart.min.js"></script>
    <style>
        :root {
            --primary: #9f7aea;
            --success: #38a169;
            --warning: #ed8936;
            --error: #e53e3e;
            --info: #3182ce;
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
        }

        .container {
            max-width: 100%;
            margin: 0 auto;
        }

        .header {
            text-align: center;
            margin-bottom: 25px;
            background: linear-gradient(135deg, var(--primary), #667eea);
            color: white;
            padding: 20px;
            border-radius: 12px;
            box-shadow: 0 4px 12px rgba(159, 122, 234, 0.3);
        }

        .header h1 {
            font-size: 24px;
            font-weight: 700;
            margin-bottom: 8px;
        }

        .header p {
            opacity: 0.9;
            font-size: 14px;
        }

        .charts-container {
            display: grid;
            grid-template-columns: 2fr 1fr;
            gap: 20px;
            margin-bottom: 20px;
        }

        @media (max-width: 768px) {
            .charts-container {
                grid-template-columns: 1fr;
            }
        }

        .chart-card {
            background: var(--card);
            border-radius: 12px;
            padding: 20px;
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.05);
            border: 1px solid var(--border);
        }

        .chart-title {
            font-size: 16px;
            font-weight: 600;
            margin-bottom: 20px;
            color: var(--text);
            padding-bottom: 10px;
            border-bottom: 2px solid var(--border);
        }

        .chart-wrapper {
            position: relative;
            height: 300px;
        }

        .sensor-list {
            display: flex;
            flex-direction: column;
            gap: 15px;
        }

        .sensor-item {
            background: linear-gradient(135deg, rgba(159, 122, 234, 0.1), rgba(102, 126, 234, 0.1));
            border-radius: 10px;
            padding: 15px;
            border: 1px solid rgba(159, 122, 234, 0.2);
        }

        .sensor-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 10px;
        }

        .sensor-name {
            font-weight: 600;
            color: var(--primary);
            font-size: 16px;
        }

        .sensor-time {
            font-size: 12px;
            color: var(--text-light);
        }

        .sensor-values {
            display: grid;
            grid-template-columns: 1fr 1fr 1fr;
            gap: 10px;
        }

        .value-item {
            text-align: center;
            padding: 8px;
            background: rgba(255, 255, 255, 0.7);
            border-radius: 6px;
        }

        .value-label {
            font-size: 11px;
            color: var(--text-light);
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }

        .value-number {
            font-size: 18px;
            font-weight: 700;
            margin-top: 3px;
        }

        .temp { color: var(--error); }
        .humi { color: var(--info); }
        .ptcl { color: var(--warning); }

        .update-info {
            text-align: center;
            margin-top: 20px;
            padding: 15px;
            background: var(--card);
            border-radius: 8px;
            border: 1px solid var(--border);
            color: var(--text-light);
            font-size: 12px;
        }

        .loading {
            text-align: center;
            padding: 40px;
            color: var(--text-light);
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>실시간 센서 모니터링</h1>
            <p>온도, 습도, 파티클 농도 추이 분석</p>
        </div>

        <div class="charts-container">
            <!-- 시간별 추이 그래프 -->
            <div class="chart-card">
                <div class="chart-title">📈 시간별 센서 데이터 추이</div>
                <div class="chart-wrapper">
                    <canvas id="timeChart"></canvas>
                </div>
            </div>

            <!-- 센서별 현재 상태 -->
            <div class="chart-card">
                <div class="chart-title">📊 센서별 현재 상태</div>
                <div class="sensor-list">
                    <?php if (count($sensor_data) > 0): ?>
                        <?php foreach($sensor_data as $sensor): ?>
                            <div class="sensor-item">
                                <div class="sensor-header">
                                    <div class="sensor-name"><?= htmlspecialchars($sensor['name']) ?></div>
                                    <div class="sensor-time"><?= $sensor['time'] ?></div>
                                </div>
                                <div class="sensor-values">
                                    <div class="value-item">
                                        <div class="value-label">온도</div>
                                        <div class="value-number temp"><?= number_format($sensor['temp'], 1) ?>°</div>
                                    </div>
                                    <div class="value-item">
                                        <div class="value-label">습도</div>
                                        <div class="value-number humi"><?= number_format($sensor['humi'], 1) ?>%</div>
                                    </div>
                                    <div class="value-item">
                                        <div class="value-label">파티클</div>
                                        <div class="value-number ptcl"><?= number_format($sensor['ptcl'], 0) ?></div>
                                    </div>
                                </div>
                            </div>
                        <?php endforeach; ?>
                    <?php else: ?>
                        <div class="loading">
                            <h3>센서 데이터 로딩 중...</h3>
                            <p>데이터를 가져오고 있습니다.</p>
                        </div>
                    <?php endif; ?>
                </div>
            </div>
        </div>

        <div class="update-info">
            마지막 업데이트: <?= date('Y년 m월 d일 H:i:s') ?> | 
            데이터 범위: 최근 24시간 | 
            총 센서 수: <?= count($sensor_data) ?>개
        </div>
    </div>

    <script>
        // 차트 데이터 준비
        const timeData = <?= json_encode($time_data) ?>;
        const sensorData = <?= json_encode($sensor_data) ?>;

        // 시간별 추이 차트
        const ctx = document.getElementById('timeChart').getContext('2d');
        
        const labels = timeData.map(item => item.time_label);
        const tempData = timeData.map(item => parseFloat(item.avg_temp) || 0);
        const humiData = timeData.map(item => parseFloat(item.avg_humi) || 0);
        const ptclData = timeData.map(item => parseFloat(item.avg_ptcl) || 0);

        new Chart(ctx, {
            type: 'line',
            data: {
                labels: labels,
                datasets: [
                    {
                        label: '온도 (°C)',
                        data: tempData,
                        borderColor: '#e53e3e',
                        backgroundColor: 'rgba(229, 62, 62, 0.1)',
                        borderWidth: 3,
                        fill: true,
                        tension: 0.4,
                        pointBackgroundColor: '#e53e3e',
                        pointBorderColor: '#ffffff',
                        pointBorderWidth: 2,
                        pointRadius: 5
                    },
                    {
                        label: '습도 (%)',
                        data: humiData,
                        borderColor: '#3182ce',
                        backgroundColor: 'rgba(49, 130, 206, 0.1)',
                        borderWidth: 3,
                        fill: true,
                        tension: 0.4,
                        pointBackgroundColor: '#3182ce',
                        pointBorderColor: '#ffffff',
                        pointBorderWidth: 2,
                        pointRadius: 5
                    },
                    {
                        label: '파티클 (ug)',
                        data: ptclData,
                        borderColor: '#ed8936',
                        backgroundColor: 'rgba(237, 137, 54, 0.1)',
                        borderWidth: 3,
                        fill: true,
                        tension: 0.4,
                        pointBackgroundColor: '#ed8936',
                        pointBorderColor: '#ffffff',
                        pointBorderWidth: 2,
                        pointRadius: 5,
                        yAxisID: 'y1'
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                interaction: {
                    intersect: false,
                    mode: 'index'
                },
                plugins: {
                    legend: {
                        position: 'top',
                        labels: {
                            usePointStyle: true,
                            padding: 20,
                            font: {
                                size: 12,
                                weight: '600'
                            }
                        }
                    },
                    tooltip: {
                        backgroundColor: 'rgba(0, 0, 0, 0.8)',
                        titleColor: '#ffffff',
                        bodyColor: '#ffffff',
                        borderColor: 'rgba(159, 122, 234, 0.5)',
                        borderWidth: 1,
                        cornerRadius: 8,
                        displayColors: true
                    }
                },
                scales: {
                    x: {
                        display: true,
                        title: {
                            display: true,
                            text: '시간',
                            font: {
                                size: 14,
                                weight: '600'
                            }
                        },
                        grid: {
                            color: 'rgba(0, 0, 0, 0.1)'
                        }
                    },
                    y: {
                        type: 'linear',
                        display: true,
                        position: 'left',
                        title: {
                            display: true,
                            text: '온도 (°C) / 습도 (%)',
                            font: {
                                size: 14,
                                weight: '600'
                            }
                        },
                        grid: {
                            color: 'rgba(0, 0, 0, 0.1)'
                        }
                    },
                    y1: {
                        type: 'linear',
                        display: true,
                        position: 'right',
                        title: {
                            display: true,
                            text: '파티클 (ug)',
                            font: {
                                size: 14,
                                weight: '600'
                            }
                        },
                        grid: {
                            drawOnChartArea: false,
                        },
                    }
                },
                elements: {
                    point: {
                        hoverRadius: 8
                    }
                },
                animation: {
                    duration: 1000,
                    easing: 'easeInOutQuart'
                }
            }
        });

        // 자동 새로고침 처리
        window.addEventListener('focus', function() {
            setTimeout(() => {
                location.reload();
            }, 100);
        });

        // 실시간 업데이트 시뮬레이션 (옵션)
        setInterval(() => {
            // 부모 페이지의 자동 새로고침 기능을 통해 처리됨
        }, 30000);
    </script>
</body>
</html>
