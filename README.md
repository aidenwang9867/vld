# VLD With JSON Support

此仓库fork自[derickr/vld](https://github.com/derickr/vld.git)，在尽量不改变已有代码的前提下，上添加了json输出支持。

## 原README

移步至[README.rst](./README.rst)。

## 起因

为了对PHP源码做XSS检测，需要提取vld输出信息做分析。vld的原版信息输出格式不方便做提取，故自行为其添加json输出。

## JSON输出

现有test.php内容如下

```php
<!DOCTYPE html>
<html>
<body>
<?php
$array = array();
$array[] = 'safe' ;
$array[] = $_GET['userData'] ;
$array[] = 'safe' ;
$tainted = $array[1] ;
$tainted += 0.0 ;
echo "<div ". $tainted ."= bob />" ;
?>
<h1>Hello World!</h1>
</div>
</body>
</html>
```

执行`php -dvld.active=1 -dvld.execute=0 -dvld.dump_json=1 test.php`得到以下输出。

```json
[
    {
        "filename": "/home/dev/sampletest.php",
        "function name": null,
        "number of ops": 18,
        "compiled vars": {
            "nums": 2,
            "vars": ["array", "tainted"]
        },
        "ops": {
            "line": [1, 5, 6, 6, 7, 7, 7, 7, 8, 8, 9, 9, 11, 14, 14, 14, 16, 20],
            "#": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17],
            "op": ["ECHO", "ASSIGN", "ASSIGN_DIM", "OP_DATA", "FETCH_R", "FETCH_DIM_R", "ASSIGN_DIM", "OP_DATA", "ASSIGN_DIM", "OP_DATA", "FETCH_DIM_R", "ASSIGN", "ASSIGN_ADD", "CONCAT", "CONCAT", "ECHO", "ECHO", "RETURN"],
            "fetch": [null, null, null, null, "global", null, null, null, null, null, null, null, null, null, null, null, null, null],
            "ext": [null, null, null, null, null, null, null, null, null, null, null, null, "  0", null, null, null, null, null],
            "return": [null, null, null, null, "$5", "$6", null, null, null, null, "$8", null, null, "~11", "~12", null, null, null],
            "op1": ["%3C%21DOCTY+html%3E%0A%3Chtml%3E%0A%3Cbody%3E%0A", "!0", "!0", "safe", "_GET", "$5", "!0", "$6", "!0", "safe", "!0", "!1", "!1", "%3Cdiv+", "~11", "~12", "%3Ch1%3EHel+World%21%3C%2Fh1%3E%0A%3C%2Fdiv%3E%0A%3C%2Fbo%3E%0A%3C%2Fhtml%3E%0A", "1"],
            "op2":  [null, "<array>", null, null, null, "userData", null, null, null, null, "1", "$8", "0", "!1", "%3D+bob+%2F%3E", null, null, null]
        },
        "paths": [[0]]
    }
]
```
相比原版纯文本输出，对编程调用更为友好。

## TODO

- [ ] 补上原版输出中的branch info内容。
- [ ] 目前json输出走stdout，计划添加dump至文件的选项。