<?php
$url = $_GET['u'];
$parts = parse_url($url);
parse_str($parts['query'], $query);
$key = $query['key'];
if($key == "YOUR API KEY HERE")
{
	echo file_get_contents("$url");
}
?>