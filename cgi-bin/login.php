#!/usr/bin/php-cgi
<?php
header("Content-Type: text/html");

$username = $_POST['username'] ?? '';
$password = $_POST['password'] ?? '';

if ($username && $password) {
    // Définir un cookie de session (expire dans 1 heure)
    $session_id = bin2hex(random_bytes(16));
    setcookie('webserv_session', $session_id, time() + 3600, '/');
    setcookie('webserv_user', $username, time() + 3600, '/');
    
    echo "<!DOCTYPE html><html><head>";
    echo "<meta http-equiv='refresh' content='0;url=/session.html'>";
    echo "</head><body>Redirecting...</body></html>";
} else {
    echo "<!DOCTYPE html><html><head><title>Error</title></head>";
    echo "<body><h1>Invalid credentials</h1></body></html>";
}
?>
