# FirewallMgr

## 简介

FirewallMgr是一个轻量级的 Windows 防火墙管理工具，允许您通过简单的配置文件为指定应用程序创建出站阻断规则。它能够根据 IP 地址、域名或全局禁网来控制特定程序的网络访问，并支持规则的批量添加、删除与查看。

## 主要功能

- **按 IP 阻断**：禁止程序访问特定 IPv4/IPv6 地址
- **按域名阻断**：禁止程序解析并访问特定域名
- **全局阻断**：禁止程序的所有出站网络访问
- **规则生命周期管理**：支持添加、删除、清空、列出本工具创建的所有规则
- **自动清理残留规则**：初始化时自动删除之前可能残留的规则

## 编译与依赖

### 环境要求

- Windows 7 / 8 / 10 / 11
- Visual Studio 2022

## 使用方法

### 命令行语法

```
FirewallMgr.exe [选项]
```

### 选项说明

| 选项                     | 说明                                                         |
| ------------------------ | ------------------------------------------------------------ |
| (无参数)                 | 仅初始化防火墙控制器，并自动清理残留规则（不执行任何阻断规则） |
| `-config`                | 读取当前目录下的 `config.txt` 并执行其中定义的阻断规则       |
| `-add <配置文件路径>`    | 从指定的配置文件中读取规则并添加到防火墙                     |
| `-delete <配置文件路径>` | 根据配置文件中的条目删除本工具创建的匹配规则                 |
| `-clear`                 | 删除所有由 FirewallMgr创建的防火墙规则                       |
| `-list`                  | 列出所有由 FirewallMgr创建的防火墙规则（详细状态）           |
| `-help`<br>`-h`<br>`/?`  | 显示帮助信息                                                 |

### 配置文件格式

配置文件为纯文本，每行一条规则，格式如下：

```
"程序完整路径" 目标
```

- **程序路径**：需要用双引号括起来
- **目标**：可以是以下三种之一
  - `ALL`（不区分大小写）：完全禁止该程序的所有出站流量
  - IP 地址：例如 `8.8.8.8`，`2400:da00::6666`，也支持子网掩码（如 `192.168.1.0/24`）
  - 域名：例如 `example.com`，`*.google.com`（支持通配符，取决于底层防火墙实现）

#### 配置文件示例（config.ini）

```ini
"c:\Program Files\SomeApp\app.exe" 8.8.8.8
"E:\tools\updater.exe" update.check.com
"C:\Users\Public\game.exe" 1.1.1.1
```

### 规则命名规则

所有由 FirewallMgr创建的防火墙规则均以 `NC_FirewallMgr_` 为前缀，后跟随机生成的唯一标识符，例如：

```
NC_FirewallMgr_7B3F9A2E_BlockIP_8.8.8.8
```

这使得工具可以安全地管理自己的规则，而不会影响其他应用程序创建的防火墙规则。

## 典型工作流

### 1. 初始化并清理残留规则

```cmd
FirewallMgr.exe
```

此命令将：
- 初始化防火墙控制器
- 删除所有标记为 `NC_FirewallMgr_` 前缀的规则（即之前由本工具创建但可能未被清理的规则）
- 报告当前规则统计

### 2. 添加一批阻断规则

编辑 `my_rules.ini`，然后执行：

```cmd
FirewallMgr.exe -add my_rules.ini
```

### 3. 查看当前生效的规则

```cmd
FirewallMgr.exe -list
```

输出示例：
```
匹配前缀: NC_FirewallMgr_
========================================

[1] NC_FirewallMgr_3F8D2A1E_BlockIP_1.1.1.1
  程序: C:\Users\Public\game.exe
  类型: IP
  方向: 出站
  状态: 已启用
  远程IP: 1.1.1.1

共找到 1 条规则
```

### 4. 删除特定的规则集

如果您想删除之前通过某个配置文件添加的规则，可以使用相同的配置文件执行删除：

```cmd
FirewallMgr.exe -delete my_rules.ini
```

> **注意**：删除时使用“程序路径 + 目标”作为匹配键，会删除所有名称以 `NC_FirewallMgr_` 开头且规则内容完全匹配的规则。

### 5. 完全清理所有规则

```cmd
FirewallMgr.exe -clear
```

### 6. 直接从默认配置 config.txt 执行

```cmd
FirewallMgr.exe -config
```

## 注意事项

1. **需要管理员权限**：修改 Windows 防火墙规则需要提升权限，请以管理员身份运行 FirewallMgr。
2. **防火墙服务必须运行**：确保 Windows Firewall 服务（MpsSvc）处于运行状态。
3. **域名阻断依赖 DNS 解析**：阻断域名时，防火墙规则会阻止对解析后 IP 的访问，若应用程序使用 DoH 或其他加密 DNS，可能需要额外配置。
4. **规则冲突**：如果存在允许规则优先级更高，阻断规则可能不生效。请检查 Windows 防火墙的规则排序。
5. **通配符域名**：底层防火墙支持一级通配符（如 `*.example.com`）

## 许可证

本项目采用 [MIT 许可证](../licenses/LICENSE-FirewallMgr)。您可以自由使用、修改和分发，但需保留原始版权声明。

WinUtils:  [MIT 许可证](../licenses/LICENSE-WinUtils)

hash-library: [zlib 许可证](../licenses/LICENSE-hash-library)

cpp-httplib: [MIT 许可证](../licenses/LICENSE-cpp-httplib)

mINI: [MIT 许可证](../licenses/LICENSE-mINI)

WinReg: [MIT 许可证](../licenses/LICENSE-WinReg)
