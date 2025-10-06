#!/usr/bin/php-cgi
<?php
header("Content-Type: text/html");

// Supprimer les cookies
setcookie('webserv_session', '', time() - 3600, '/');
setcookie('webserv_user', '', time() - 3600, '/');

echo "<!DOCTYPE html><html><head>";
echo "<meta http-equiv='refresh' content='0;url=/login.html'>";
echo "</head><body>Logged out...</body></html>";
?>
