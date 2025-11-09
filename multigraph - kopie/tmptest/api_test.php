<?php
$ch = curl_init("http://localhost:8080/api_jsonrpc.php");
curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
curl_setopt($ch, CURLOPT_POST, true);
curl_setopt($ch, CURLOPT_HTTPHEADER, ["Content-Type: application/json-rpc"]);
curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode([
    "jsonrpc" => "2.0",
    "method" => "apiinfo.version",
    "params" => [],
    "id" => 1
]));
$result = curl_exec($ch);
curl_close($ch);
echo $result . "\n";
