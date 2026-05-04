#include "TaggedFirewallController.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <WinUtils/WinUtils.h>
#include <WinUtils/Console.h>
#include "WinUtils/StrConvert.h"
#include "WinUtils/CmdParser.h"

using namespace std;
using namespace WinUtils;

// 配置项结构
struct ConfigEntry {
    wstring programPath;
    wstring target;
    wstring type;
};

// 解析单行配置
ConfigEntry ParseConfigLine(const wstring& line) {
    ConfigEntry entry;
    if (line.empty() || line[0] == L';' || line[0] == L'#') return entry;

    size_t firstQuote = line.find(L'\"');
    if (firstQuote == wstring::npos) return entry;

    size_t secondQuote = line.find(L'\"', firstQuote + 1);
    if (secondQuote == wstring::npos) return entry;

    entry.programPath = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);

    size_t targetStart = secondQuote + 1;
    while (targetStart < line.length() && line[targetStart] == L' ') targetStart++;
    entry.target = line.substr(targetStart);

    while (!entry.target.empty() &&
        (entry.target.back() == L' ' || entry.target.back() == L'\r' || entry.target.back() == L'\n')) {
        entry.target.pop_back();
    }

    if (entry.target == L"ALL" || entry.target == L"all" || entry.target == L"All") {
        entry.type = L"ALL";
    }
    else {
        bool hasAlpha = false;
        for (wchar_t c : entry.target) {
            if ((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z')) {
                hasAlpha = true;
                break;
            }
        }
        if (entry.target.find(L":") != wstring::npos ||
            (entry.target.find(L".") != wstring::npos && hasAlpha)) {
            entry.type = hasAlpha ? L"DOMAIN" : L"IP";
        }
        else {
            entry.type = L"IP";
        }
    }

    return entry;
}

// 读取TXT配置文件
std::vector<ConfigEntry> ReadConfigFile(const std::wstring& filename) {
    std::vector<ConfigEntry> entries;
    std::ifstream file(filename);
    if (!file.is_open()) {
        return entries;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        std::wstring wline = Utf8ToWide(line);
        if (wline.empty()) {
            continue;
        }
        if (wline[0] == L'\uFEFF') {
            wline = wline.substr(1);
        }
        ConfigEntry entry = ParseConfigLine(wline);
        if (!entry.programPath.empty()) {
            entries.push_back(std::move(entry));
        }
    }
    return entries;
}

// 执行配置
void ExecuteConfig(TaggedFirewallController& controller, const vector<ConfigEntry>& entries) {
    wcout << L"开始执行配置，共 " << entries.size() << L" 条规则" << endl;

    for (const auto& entry : entries) {
        wcout << L"\n处理: " << entry.programPath << L" -> " << entry.target << endl;

        wstring ruleName;
        if (entry.type == L"ALL") {
            ruleName = controller.BlockAllAccess(entry.programPath);
        }
        else if (entry.type == L"DOMAIN") {
            ruleName = controller.BlockDomainAccess(entry.programPath, entry.target,
                TaggedFirewallController::Direction::OUTBOUND);
        }
        else {
            ruleName = controller.BlockIpAccess(entry.programPath, entry.target,
                TaggedFirewallController::Direction::OUTBOUND);
        }

        if (!ruleName.empty()) {
            wcout << L"  成功: " << ruleName << endl;
        }
        else {
            wcerr << L"  失败" << endl;
        }
    }

    wcout << L"\n配置执行完成" << endl;
}

// 列出所有NC_开头的规则
void ListRules(const wstring& appName, TaggedFirewallController& controller) {
    auto allRules = controller.GetAllRules();

    wstring prefix = wstring(L"NC_") + appName + L"_";

    wcout << L"\n匹配前缀: " << prefix << endl;
    wcout << L"========================================" << endl;

    int count = 0;
    for (const auto& rule : allRules) {
        if (rule.ruleName.find(prefix) == 0) {
            count++;
            wcout << L"\n[" << count << L"] " << rule.ruleName << endl;
            wcout << L"  程序: " << (rule.targetProgram.empty() ? L"(空)" : rule.targetProgram) << endl;
            wcout << L"  类型: " << rule.ruleType << endl;
            wcout << L"  方向: " << (rule.direction == TaggedFirewallController::Direction::INBOUND ? L"入站" : L"出站") << endl;
            wcout << L"  状态: " << (rule.enabled ? L"已启用" : L"已禁用") << endl;
            if (!rule.remoteAddresses.empty() && rule.remoteAddresses != L"any") {
                wcout << L"  远程IP: " << rule.remoteAddresses << endl;
            }
            if (!rule.remotePorts.empty() && rule.remotePorts != L"any") {
                wcout << L"  远程端口: " << rule.remotePorts << endl;
            }
        }
    }

    if (count == 0) {
        wcout << L"\n未找到匹配的规则" << endl;
    }
    else {
        wcout << L"\n========================================" << endl;
        wcout << L"共找到 " << count << L" 条规则" << endl;
    }
}

// 显示帮助
void ShowHelp(const wchar_t* programName) {
    wcout << L"用法: " << programName << L" [选项]" << endl;
    wcout << L"选项:" << endl;
    wcout << L"  (无参数)    仅初始化防火墙，清理残留规则" << endl;
    wcout << L"  -config     从config.txt读取配置并执行阻断规则" << endl;
    wcout << L"  -add <文件> 从指定配置文件添加阻断规则" << endl;
    wcout << L"  -delete <文件> 从指定配置文件删除对应规则" << endl;
    wcout << L"  -clear      删除所有本应用创建的规则" << endl;
    wcout << L"  -list       列出所有本应用创建的防火墙规则" << endl;
    wcout << L"  -help       显示此帮助信息" << endl;
    wcout << endl;
    wcout << L"配置文件格式:" << endl;
    wcout << L"  \"程序路径\" 目标" << endl;
    wcout << L"目标可以是: IP地址、域名、或ALL" << endl;
}

int wmain(int argc, wchar_t* argv[]) {
    wstring fullCmdLine = GetCommandLineW();
    wstring args = ExtractArguments(fullCmdLine);

    CmdParser parser(true);
    if (!parser.parse(args, CmdParser::ParseMode::Normal)) {
        wcerr << L"命令行解析失败" << endl;
        return 1;
    }

    const auto& cmdMap = parser.result();
    enum class Mode { INIT_ONLY, CONFIG, ADD, DELETE1, CLEAR, LIST, HELP };
    Mode mode = Mode::INIT_ONLY;
    wstring configFile;

    // 定义支持的子命令
    auto hasCmd = [&](const wstring& name) -> bool {
        return cmdMap.find(name) != cmdMap.end();
        };
    auto getFirstParam = [&](const wstring& name) -> wstring {
        auto it = cmdMap.find(name);
        if (it != cmdMap.end() && !it->second.empty())
            return it->second[0];
        return L"";
        };

    if (hasCmd(L"help") || hasCmd(L"h") || hasCmd(L"?")) {
        mode = Mode::HELP;
    }
    else if (hasCmd(L"config")) {
        mode = Mode::CONFIG;
		configFile = ResolvePath(L"config.txt");
    }
    else if (hasCmd(L"add")) {
        mode = Mode::ADD;
        configFile = getFirstParam(L"add");
        if (configFile.empty()) {
            wcerr << L"错误: -add 需要指定配置文件路径" << endl;
            ShowHelp(argv[0]);
            return 1;
        }
        configFile = ResolvePath(configFile);
    }
    else if (hasCmd(L"delete")) {
        mode = Mode::DELETE1;
        configFile = getFirstParam(L"delete");
        if (configFile.empty()) {
            wcerr << L"错误: -delete 需要指定配置文件路径" << endl;
            ShowHelp(argv[0]);
            return 1;
        }
        configFile = ResolvePath(configFile);
    }
    else if (hasCmd(L"clear")) {
        mode = Mode::CLEAR;
    }
    else if (hasCmd(L"list")) {
        mode = Mode::LIST;
    }
    else if (!cmdMap.empty()) {
        wcerr << L"未知参数" << endl;
        ShowHelp(argv[0]);
        return 1;
    }

    TaggedFirewallController::ControllerIdentity id;
    id.appName = L"NetBlocker";
    id.version = L"1.0";
    TaggedFirewallController controller(id);

    wcout << L"正在初始化网络防火墙控制器..." << endl;
    if (!controller.Initialize()) {
        wcerr << L"初始化失败" << endl;
        return 1;
    }
    wcout << L"初始化成功" << endl;
    // -help 模式
    if (mode == Mode::HELP) {
        ShowHelp(argv[0]);
        return 0;
    }

    // -list 模式
    if (mode == Mode::LIST) {
        ListRules(L"NetBlocker", controller);
        return 0;
    }

    // -clear 模式
    if (mode == Mode::CLEAR) {
        wcout << L"正在删除所有本应用创建的规则..." << endl;
        if (controller.CleanupResidualRules()) {
            wcout << L"删除成功" << endl;
        }
        else {
            wcerr << L"删除失败" << endl;
        }
        return 0;
    }

	// -config 模式
    if (mode == Mode::CONFIG) {
		configFile = ResolvePath(L"config.txt");
		mode = Mode::ADD;
    }

    // -add 模式
    if (mode == Mode::ADD) {
        wcout << L"正在从 " << configFile << L" 读取配置并添加规则..." << endl;
        auto entries = ReadConfigFile(configFile);
        if (entries.empty()) {
            wcout << L"配置文件为空或不存在" << endl;
        }
        else {
            ExecuteConfig(controller, entries);
        }
    }

    // -delete 模式
    else if (mode == Mode::DELETE1) {
        wcout << L"正在从 " << configFile << L" 读取配置并删除规则..." << endl;
        auto entries = ReadConfigFile(configFile);
        if (entries.empty()) {
            wcout << L"配置文件为空或不存在" << endl;
        }
        else {
            wcout << L"开始删除规则..." << endl;
            int successCount = 0;
            for (const auto& entry : entries) {
                wstring targetSpecifier;
                if (entry.type == L"ALL") targetSpecifier = L"ALL";
                else if (entry.type == L"DOMAIN") targetSpecifier = entry.target;
                else targetSpecifier = entry.target; // IP
                if (controller.DeleteRulesByTarget(entry.programPath, targetSpecifier)) {
                    successCount++;
                }
            }
            wcout << L"成功删除 " << successCount << L" 条规则（按配置条目计）" << endl;
        }
    }
    else {
        wcout << L"模式: 仅初始化" << endl;
        wcout << L"使用 -add/-delete/-clear 管理规则，-list 查看规则" << endl;
    }

    // 显示统计
    if (mode != Mode::LIST && mode != Mode::CLEAR && mode != Mode::DELETE1) {
        auto stats = controller.GetStatistics();
        wcout << L"\n当前规则统计:" << endl;
        wcout << L"  本实例规则: " << stats.totalMyRules << endl;
        wcout << L"  本应用规则: " << stats.totalOurRules << endl;
        wcout << L"  已启用: " << stats.enabledRules << endl;
    }

    return 0;
}