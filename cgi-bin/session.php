#!/usr/bin/php-cgi
<?php
$session_id = $_COOKIE['webserv_session'] ?? '';
$username = $_COOKIE['webserv_user'] ?? '';
?>
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Session - Webserv</title>
    <link rel="stylesheet" href="/css/session.css">
</head>
<body>
    <div class="container">
        <?php if ($session_id && $username): ?>
            <h1>Welcome, <?php echo htmlspecialchars($username); ?>!</h1>
            <div class="info-card">
                <p><strong>Username:</strong> <?php echo htmlspecialchars($username); ?></p>
                <p><strong>Session ID:</strong> <?php echo htmlspecialchars($session_id); ?></p>
            </div>
            <div class="links">
                <a href="/" class="link-btn">Home</a>
                <a href="/upload.html" class="link-btn">Upload Files</a>
                <a href="/cgi-test.html" class="link-btn">CGI Tests</a>
                <a href="/cgi-bin/logout.php" class="link-btn logout">Logout</a>
            </div>
        <?php else: ?>
            <h1>No Active Session</h1>
            <p>Please log in first.</p>
            <a href="/login.html" class="link-btn">Go to Login</a>
        <?php endif; ?>
    </div>
</body>
</html>