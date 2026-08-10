# Downloader
This is an AI agent self-downloading project(I create it for an competition) It may have some problem(If your program cannot run,please email us at Simba3408@77.ink)If you like this program please give me a star  

Creater:
This project is created by Simba3405 from Chengdu No.7 YuCai high school,China
You can sreach me as "Geek236" on BiliBili

Thanks for e.. only myself

Ok,next is the introduction file(if I have some spelling mistake ,please do not blame me)

# Downloader Smart Download Assistant —— Technical Documentation

**By Squarcal Technology® Downloader™ Project Team**

## I. Project Overview

### 1.1 Project Background

In the digital age, software installation and file downloading are among the most frequent user operations. However, average users face numerous difficulties:

- **Inability to find official download sources** — Search engine results are cluttered, making it hard to distinguish genuine sites from fakes.
- **Ad traps on download sites** — Domestic download platforms commonly have bundled software and deceptive buttons.
- **High threshold for command-line tools** — Package managers like `winget` and `choco` require memorizing complex commands.
- **Complex network environments** — Significant network disparities between domestic and international regions restrict access to certain resources.

### 1.2 Project Positioning

This project is an intelligent download **Agent** based on a Large Language Model (LLM). Users need only describe their needs in natural language (e.g., "help me download Google"), and the AI autonomously completes the entire process of searching, analyzing, and downloading.

## II. System Architecture

### 2.1 Overall Architecture Diagram
User Interaction Layer (Console UI)
│
├── Colorful Terminal / Settings Menu / Command Parsing
▼
AI Decision Engine (LLM API)
│
├── API → Natural Language Understanding → Structured Action Tag Output
▼
Action Execution Layer
│
├── <cmd> → Command Execution (winget, choco, system commands)
├── <download> → File Download (direct URL to installer)
├── <fetch> → Web Page Fetching (parse pages, extract real links)
└── <network> → Search (via SearXNG multi-engine search)
▼
Data Persistence Layer (SQLite)
│
└── Session Logs / Operation Audits / History Replay / Error Analysis



## III. Core Technical Innovations

### 3.1 Tagged Action Protocol (Self-coined Term)

**Technical Principle:** Constrain the AI's free-text output into structured tags, achieving a separation of "thinking" and "acting."

```xml
<thinking>Analyzing current state...</thinking>
<cmd>winget install Scratch</cmd>
Advantages:

Deterministic Execution: The program can precisely parse the AI's intent, avoiding ambiguity in natural language.

Auditable Traceability: Every action has a clear type and content, facilitating logging.

Tag System:

Tag	Function	Use Case
<thinking>	Reasoning	AI's internal analysis
<cmd>	Command execution	Invoke winget, choco, system commands
<download>	File download	Direct download of installer from URL
<fetch>	Web page fetching	Parse download pages to extract real links
<network>	Search	Multi-engine search via SearXNG
<decision>	User decision	Multi-source selection, sensitive ops
<done>	Task completion	Mark the end of the workflow
3.2 Intelligent Web Link Extraction Engine
Technical Challenge: Download buttons on domestic sites commonly employ anti-scraping mechanisms such as JavaScript redirects, data-url hiding, and class name obfuscation, making traditional regex extraction ineffective.

Solution — Multi-Layer Parsing Strategy:

Layer 1: Standard <a href> tag extraction.

Layer 2: onclick="window.open('url')" event extraction.

Layer 3: data-url custom attribute extraction.

Layer 4: Semantic class*="download" extraction.

Layer 5: Bare link text matching (e.g., https://.../*.exe).

Output Format Optimization:

text
>>> DIRECT DOWNLOAD LINKS (HIGHEST PRIORITY) <<<
[FILE] https://dl.example.com/ScratchSetup.exe

>>> DOWNLOAD BUTTONS <<<
[DOWNLOAD] 立即下载
URL: https://example.com/download?id=123
3.3 Adaptive History Memory Mechanism
Technical Challenge: The AI's context window is limited; maintaining full history causes token explosion, while over-pruning may cause the AI to "forget" critical information (e.g., download links).

Solution — Layered History Strategy:

Rounds	Recording Precision	Reason
Last 2 rounds	Full results (up to 1500 chars)	AI needs to know exactly what links were just found.
Rounds 3–5	Brief summary (~100 chars)	Understand failure reasons to avoid repeating mistakes.
Earlier rounds	Not displayed	Prevent information overload.
python
if (rowIndex < 2):
    # Full results: include all links and page content
    result += " Full: " + fullResult + "\n"
else:
    # Brief: only action type + success/failure status
    result += " Brief: " + brief + "\n"
3.4 Aggregated Search Middleware (Self-hosted SearXNG)
Technical Principle: A self-hosted SearXNG instance aggregates results from Baidu, Bing, and other engines, outputting unified JSON format.

Architectural Advantages:

Decentralization: Does not rely on a single search engine, avoiding API restrictions.

Privacy Protection: Search requests are processed locally without passing through third-party logs.

Domestic Adaptation: Only enables engines accessible within China (Baidu, Bing), disabling Google and DuckDuckGo.

Simple Deployment: No need to manage complex Docker setups.

IV. Key Technical Challenges and Solutions
Challenge 1: AI Infinite Loop
Symptoms: The AI repeatedly fetches the same page or repeats the same search query.

Root Cause Analysis:

Excessively long history → Token limit exceeded → AI "loses memory."

Lack of termination conditions → Unlimited retries.

Failure to recognize repetitive patterns → Continuous repetition of the same errors.

Solutions:

Maximum round limit (MAX_ROUNDS = 15).

Anti-loop rules embedded in system prompt:

text
[ANTI-LOOP RULES]
- If the last 3 actions all FAILED, switch approach.
- NEVER output the same tag content twice.
Layered history compression (full for last 2 rounds, brief summaries for the rest).

Challenge 2: Download Site Anti-Scraping and Dynamic Loading
Symptoms: The fetched HTML contains no visible download link.

Root Cause Analysis:

Real links are hidden within onclick events or data-url attributes.

Some links are dynamically generated by JavaScript, which static fetching cannot capture.

Download buttons use obfuscated class names (e.g., btn-dl-x7k9).

Solutions:

Multi-layer regex parsing (standard tags + events + attributes + semantic classes).

Bare link text matching (directly search HTML for http://.../*.exe patterns).

Page title extraction (to assist the AI in judging page validity).

Challenge 3: Domestic vs. International Network Discrepancies
Symptoms: Timeouts when accessing Google, GitHub, or other official international sites.

Root Cause Analysis:

DNS pollution or IP blocking.

SSL certificate verification failures.

International bandwidth throttling.

Solutions:

SearXNG only enables domestically accessible engines (Baidu, Bing).

System prompt explicitly instructs the AI: "Avoid sites from us, uk".

Extend download timeout to 60 seconds and support redirect following.

V. Project Advantages
Lightweight software, easy to use.

Facilitates batch software installation.

Suitable for installing C++ libraries.

Natural language interaction.

VI. Application Scenarios
Novice users: Unfamiliar with winget; simply say "install Chrome."

Coding enthusiasts: Install numerous libraries quickly.

Batch deployment: IT administrators quickly set up software on multiple machines.

Restricted networks: Automatically select domestically available download sources.

VII. Future Roadmap
Feature	Description
GUI Automation	Combine screenshot capture + OCR to automatically click real download buttons on sites.
Smart Installation	Automatically run installers and skip bundled software checkboxes.
Plugin System	Support user-defined download sources (e.g., internal company mirrors).
Cross-Platform	Port to Linux (apt/dnf) and macOS (brew).
VIII. Conclusion
This project seamlessly bridges the reasoning capabilities of Large Language Models with system-level operations through a Tagged Action Protocol. Combined with intelligent webpage parsing and adaptive history memory, it addresses the core pain points of traditional download tools—"can't find, can't download correctly, can't install properly." In our information-rich era, this technology saves users considerable time.

IX. Call to Action
This project will be developed and maintained long-term, distributed freely to everyone, and will feature extensive user customization capabilities.

We envision Downloader changing everyone's life in the future.

If this project has helped you, please consider buying us a coffee at Squarcal.com.

If you are interested in acquiring the project team and all intellectual property rights of the entire project, please send an email to bSimba3405@outlook.com.

For more information, please visit www.Squarcal.com.
（HAHA，you have been tricked,We do not have a website,or why I have to upload it to GITHUB.I'm only a 13-year-old teenager,I do not have such that money）

