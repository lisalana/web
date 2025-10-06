#!/usr/bin/php-cgi
<?php
header("Content-Type: text/html");
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>PHP CGI Result</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 40px 20px;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: white;
            border-radius: 20px;
            padding: 40px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
        }
        h1 { color: #2d3748; margin-bottom: 30px; }
        .section {
            background: #f7fafc;
            padding: 20px;
            border-radius: 12px;
            margin-bottom: 20px;
        }
        h2 { color: #4a5568; font-size: 1.3em; margin-bottom: 15px; }
        table { width: 100%; border-collapse: collapse; }
        th, td { padding: 12px; text-align: left; border-bottom: 1px solid #e2e8f0; }
        th { background: #667eea; color: white; font-weight: 600; }
        tr:hover { background: #edf2f7; }
        .back-link {
            display: inline-block;
            margin-top: 20px;
            padding: 12px 24px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            text-decoration: none;
            border-radius: 8px;
            font-weight: 600;
        }
        .back-link:hover { transform: translateY(-2px); }
    </style>
</head>
<body>
    <div class="container">
        <h1>PHP CGI Response</h1>
        
        <div class="section">
            <h2>Request Information</h2>
            <table>
                <tr><th>Variable</th><th>Value</th></tr>
                <tr><td>Request Method</td><td><?php echo $_SERVER['REQUEST_METHOD']; ?></td></tr>
                <tr><td>Query String</td><td><?php echo $_SERVER['QUERY_STRING'] ?? 'N/A'; ?></td></tr>
                <tr><td>Content Type</td><td><?php echo $_SERVER['CONTENT_TYPE'] ?? 'N/A'; ?></td></tr>
                <tr><td>Content Length</td><td><?php echo $_SERVER['CONTENT_LENGTH'] ?? '0'; ?></td></tr>
            </table>
        </div>
        
        <?php if ($_SERVER['REQUEST_METHOD'] === 'POST' || !empty($_GET)): ?>
        <div class="section">
            <h2><?php echo $_SERVER['REQUEST_METHOD'] === 'POST' ? 'POST' : 'GET'; ?> Data</h2>
            <table>
                <tr><th>Key</th><th>Value</th></tr>
                <?php 
                $data = $_SERVER['REQUEST_METHOD'] === 'POST' ? $_POST : $_GET;
                foreach ($data as $key => $value): 
                ?>
                <tr><td><?php echo htmlspecialchars($key); ?></td><td><?php echo htmlspecialchars($value); ?></td></tr>
                <?php endforeach; ?>
            </table>
        </div>
        <?php endif; ?>
        
        <a href="/cgi-test.html" class="back-link">Back to Tests</a>
    </div>
</body>
</html>
