from pathlib import Path

root=Path(__file__).parent
code=(root/'build_plan.py').read_text(encoding='utf-8')
code=code.replace("'脊梁世界树之巅开发规划.md'", "'截至2026年10月5日项目完成记录.md'")
code=code.replace("'脊梁世界树之巅开发规划.docx'", "'截至2026年10月5日项目完成记录.docx'")
code=code.replace('脊梁世界树之巅开发规划与任务清单','脊梁世界树之巅项目完成记录')
code=code.replace('开发规划与任务清单','项目完成记录')
code=code.replace('基于游戏设定和当前 Unreal Engine 项目的开发路线','截至2026年10月5日的项目成果和验收边界')
code=code.replace("active_section == '05'", "active_section == 'none'")
exec(compile(code,str(root/'build_record.py'),'exec'))
