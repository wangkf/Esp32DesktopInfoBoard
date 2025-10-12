import json
import os
import requests
from datetime import datetime
from datetime import timedelta
import logging
import re

# 配置日志
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

# 获取当前目录
data_dir = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'data')

# API 参数定义
NEWS_API_URL = "https://apis.tianapi.com/bulletin/index"
# NEWS_API_KEY 使用 config.json 中的配置
ASTRONAUTS_API_URL = "http://api.open-notify.org/astros.json"
ICiba_API_URL = "https://open.iciba.com/dsapi/"
# 读取配置文件获取 NEWS_API_KEY
def get_news_api_key():
    config_path = os.path.join(data_dir, 'config.json')
    try:
        with open(config_path, 'r', encoding='utf-8') as f:
            config = json.load(f)
        api_key = config.get('api', {}).get('key', '')
        
        # 检查是否是占位符值
        if api_key == 'YOUR_API_KEY_HERE' or not api_key:
            logger.warning(f'新闻API Key是占位符值或为空，请在{config_path}中更新为有效的API Key')
        
        return api_key
    except Exception as e:
        logger.error(f'读取config.json时出错: {e}')
        return ''

NEWS_API_KEY = get_news_api_key()

# 获取当前时间作为更新时间
def get_current_time():
    return datetime.now().strftime('%Y-%m-%d %H:%M:%S')

# 检查是否需要更新文件
# file_path: 文件路径
# update_interval: 更新间隔（以小时为单位）
def should_update_file(file_path, update_interval):
    # 如果文件不存在，需要更新
    if not os.path.exists(file_path):
        logger.info(f'文件{file_path}不存在，需要更新')
        return True
    
    try:
        # 读取文件内容，获取最后更新时间
        with open(file_path, 'r', encoding='utf-8') as f:
            data = json.load(f)
        
        # 尝试从不同位置获取最后更新时间
        last_updated = None
        if isinstance(data, dict):
            # 检查news.json格式
            if 'result' in data and isinstance(data['result'], dict):
                if 'last_updated' in data['result']:
                    last_updated = data['result']['last_updated']
                # 检查aprs.json格式
                elif 'last_update' in data['result']:
                    last_updated = data['result']['last_update']
            # 检查其他格式
            elif 'last_updated' in data:
                last_updated = data['last_updated']
        
        # 如果没有找到最后更新时间，需要更新
        if not last_updated:
            logger.info(f'文件{file_path}中未找到最后更新时间，需要更新')
            return True
        
        # 解析最后更新时间
        last_update_time = datetime.strptime(last_updated, '%Y-%m-%d %H:%M:%S')
        current_time = datetime.now()
        
        # 计算时间差（小时）
        hours_diff = (current_time - last_update_time).total_seconds() / 3600
        
        # 判断是否需要更新
        if hours_diff >= update_interval:
            logger.info(f'文件{file_path}最后更新于{last_updated}，已超过{update_interval}小时，需要更新')
            return True
        else:
            logger.info(f'文件{file_path}最后更新于{last_updated}，尚未超过{update_interval}小时，不需要更新')
            return False
    except Exception as e:
        logger.error(f'检查文件{file_path}更新状态时出错: {e}')
        # 出错时默认需要更新
        return True

# 通用HTTP请求函数 - 添加自定义User-Agent支持
def http_get_request(url, params=None, headers=None):
    try:
        # 默认请求头
        default_headers = {
            'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/96.0.4664.110 Safari/537.36'
        }
        
        # 如果提供了自定义请求头，合并它们
        if headers:
            default_headers.update(headers)
            
        response = requests.get(url, params=params, headers=default_headers, timeout=10)
        if response.status_code == 200:
            return response
        else:
            logger.error(f'请求失败: {url}, 状态码: {response.status_code}')
            logger.error(f'响应内容: {response.text}')
            return None
    except Exception as e:
        logger.error(f'请求异常: {url}, 错误: {e}')
        return None

## 1. 处理iciba.json - 只保留指定字段：tts, content, note, fenxiang_img及result中的last_updated
def process_iciba():
    file_path = os.path.join(data_dir, 'iciba.json')
    try:
        # 获取当前时间
        current_time = get_current_time()
        
        # 创建一个新的空数据结构，只包含需要的字段
        filtered_data = {'result': {}}
        
        # 如果文件存在，读取现有数据
        if os.path.exists(file_path):
            with open(file_path, 'r', encoding='utf-8') as f:
                existing_data = json.load(f)
            
            # 从现有数据中提取需要的字段
            if isinstance(existing_data, dict):
                # 提取根级别的指定字段
                for field in ['tts', 'content', 'note', 'fenxiang_img']:
                    if field in existing_data:
                        filtered_data[field] = existing_data[field]
                
                # 提取result中的字段
                if 'result' in existing_data and isinstance(existing_data['result'], dict):
                    filtered_data['result'] = {}
                    # 保留last_updated字段
                    if 'last_updated' in existing_data['result']:
                        filtered_data['result']['last_updated'] = existing_data['result']['last_updated']
        
        # 尝试从API获取新数据
        response = http_get_request(ICiba_API_URL)
        api_data = None
        if response:
            try:
                api_data = response.json()
            except json.JSONDecodeError:
                logger.error(f'解析ICiba API响应时出错')
        
        if api_data and isinstance(api_data, dict):
            # 从API响应中提取需要的字段
            for field in ['tts', 'content', 'note', 'fenxiang_img']:
                if field in api_data:
                    filtered_data[field] = api_data[field]
        
        # 确保last_updated字段存在并更新为当前时间
        filtered_data['result']['last_updated'] = current_time
        
        # 保存文件，只保留过滤后的数据
        # 只有当至少有content或note字段时才保存文件
        if 'content' in filtered_data or 'note' in filtered_data:
            with open(file_path, 'w', encoding='utf-8') as f:
                json.dump(filtered_data, f, ensure_ascii=False, indent=2)
            
            logger.info(f'已成功处理iciba.json，只保留指定字段，更新时间: {current_time}')
        else:
            logger.warning(f'没有获取到有效数据，不更新iciba.json文件')
    except Exception as e:
        logger.error(f'处理iciba.json时出错: {e}')

## 2. 处理news.json - 从API获取数据，只保留title和last_updated，其他数据不保存
def process_news():
    file_path = os.path.join(data_dir, 'news.json')
    try:
        # 检查API Key是否有效
        if NEWS_API_KEY == 'YOUR_API_KEY_HERE' or not NEWS_API_KEY:
            logger.warning(f'新闻API Key无效或未设置，请在{os.path.join(data_dir, "config.json")}中更新为有效的API Key')
            # 如果文件存在，只更新时间戳
            if os.path.exists(file_path):
                current_time = get_current_time()
                with open(file_path, 'r', encoding='utf-8') as f:
                    data = json.load(f)
                # 确保数据结构正确
                if 'result' not in data:
                    data['result'] = {}
                data['result']['last_updated'] = current_time
                
                with open(file_path, 'w', encoding='utf-8') as f:
                    json.dump(data, f, ensure_ascii=False, indent=2)
                
                logger.warning(f'API Key无效，使用现有数据更新news.json时间戳: {current_time}')
            return
        
        # 构建请求URL和参数
        params = {'key': NEWS_API_KEY}
        logger.info(f'获取新闻数据: {NEWS_API_URL}')
        logger.info(f'使用的API Key: {NEWS_API_KEY}')
        
        # 发送请求
        response = http_get_request(NEWS_API_URL, params)
        
        # 初始化数据结构 - 只包含必要的字段
        data = {'result': {'list': []}}
        
        # 只有在API请求成功且包含result字段时，才使用API数据更新
        api_data = None
        if response:
            logger.info(f'新闻API响应状态码: {response.status_code}')
            try:
                api_data = response.json()
                logger.info(f'新闻API响应内容: {api_data}')
            except json.JSONDecodeError:
                logger.error(f'解析News API响应时出错，响应内容: {response.text}')
        else:
            logger.warning(f'新闻API请求失败，未获取到响应')
        
        # 添加更新时间
        current_time = get_current_time()
        
        if api_data and isinstance(api_data, dict):
            # 检查API是否返回错误码
            if 'code' in api_data and api_data['code'] != 200:
                logger.error(f'新闻API返回错误: 代码={api_data.get("code")}, 消息={api_data.get("msg")}')
                # 特殊处理key错误，提供更明确的指导
                if api_data.get('code') == 230 and 'key' in str(api_data.get('msg', '')).lower():
                    logger.warning(f'请在{os.path.join(data_dir, "config.json")}中更新为有效的API Key')
            
            # 继续处理数据，即使有错误码也尝试从中提取有用信息
            if 'result' in api_data:
                # 创建新的数据结构，只保留必要的字段
                result_data = {'list': []}
                
                # 获取新闻列表（注意接口返回的可能是'news'或'list'字段）
                news_source = api_data['result'].get('list', api_data['result'].get('news', []))
                
                if isinstance(news_source, list):
                    # 只保留前10条新闻的title
                    news_items = news_source[:10]
                    # 确保每条新闻只有title字段
                    for item in news_items:
                        if isinstance(item, dict) and 'title' in item:
                            result_data['list'].append({'title': item['title']})
                
                # 添加更新时间
                result_data['last_updated'] = current_time
                
                # 构建最终的数据结构
                data = {'result': result_data}
                
                # 保存文件
                with open(file_path, 'w', encoding='utf-8') as f:
                    json.dump(data, f, ensure_ascii=False, indent=2)
                
                logger.info(f'已成功处理news.json，只保留title和last_updated，保留了{len(result_data.get("list", []))}条新闻，更新时间: {current_time}')
        else:
            # 如果API请求失败，记录警告但不创建默认数据
            if os.path.exists(file_path):
                # 文件存在时，读取现有数据并只更新时间戳
                with open(file_path, 'r', encoding='utf-8') as f:
                    existing_data = json.load(f)
                
                # 确保数据结构正确
                if 'result' not in existing_data:
                    existing_data['result'] = {'list': []}
                existing_data['result']['last_updated'] = current_time
                
                # 只保留list和last_updated字段
                cleaned_data = {
                    'result': {
                        'list': existing_data['result'].get('list', []),
                        'last_updated': current_time
                    }
                }
                
                with open(file_path, 'w', encoding='utf-8') as f:
                    json.dump(cleaned_data, f, ensure_ascii=False, indent=2)
                
                logger.warning(f'API请求失败，使用现有数据更新news.json时间戳: {current_time}')
            else:
                logger.warning(f'API请求失败，且news.json文件不存在，不创建新文件')
    except Exception as e:
        logger.error(f'处理news.json时出错: {e}')

## 3. 处理astronauts.json - 从API获取数据并添加更新时间
def process_astronauts():
    file_path = os.path.join(data_dir, 'astronauts.json')
    try:
        logger.info(f'获取宇航员数据: {ASTRONAUTS_API_URL}')
        
        # 发送请求
        response = http_get_request(ASTRONAUTS_API_URL)
        
        api_data = None
        if response:
            try:
                api_data = response.json()
            except json.JSONDecodeError:
                logger.error(f'解析Astronauts API响应时出错')
        
        if api_data:
            # 构建符合格式的数据结构，只包含必要字段
            result = {
                'code': 200,
                'msg': 'success',
                'result': {
                    'astronauts': [],
                    'total': 0
                }
            }
            
            # 解析API响应，只保留实际获取的数据
            if 'people' in api_data:
                astronauts = []
                for person in api_data['people']:
                    # 只保留从API获取的人名和飞行器名称，不添加额外字段
                    astronaut_info = {
                        'name': person.get('name', ''),
                        'craft': person.get('craft', '')
                    }
                    astronauts.append(astronaut_info)
                
                result['result']['astronauts'] = astronauts
                result['result']['total'] = len(astronauts)
            
            # 添加更新时间
            current_time = get_current_time()
            result['result']['last_updated'] = current_time
            
            # 保存文件
            with open(file_path, 'w', encoding='utf-8') as f:
                json.dump(result, f, ensure_ascii=False, indent=2)
            
            logger.info(f'已成功更新astronauts.json，更新时间: {current_time}')
        else:
            # 如果API请求失败，使用现有的数据并更新时间戳
            if os.path.exists(file_path):
                with open(file_path, 'r', encoding='utf-8') as f:
                    data = json.load(f)
                
                current_time = get_current_time()
                if 'result' not in data:
                    data['result'] = {}
                data['result']['last_updated'] = current_time
                
                with open(file_path, 'w', encoding='utf-8') as f:
                    json.dump(data, f, ensure_ascii=False, indent=2)
                
                logger.warning(f'API请求失败，使用现有数据更新astronauts.json时间戳: {current_time}')
            else:
                logger.warning(f'API请求失败，且astronauts.json文件不存在，不创建新文件')
    except Exception as e:
        logger.error(f'处理astronauts.json时出错: {e}')

# 主函数
def main():
    logger.info('开始处理JSON文件...')    
    # 根据更新频率处理不同的文件
    def process_json_file(json_filename, update_interval, process_function):
        """统一处理JSON文件的更新逻辑"""
        file_path = os.path.join(data_dir, json_filename)
        if should_update_file(file_path, update_interval):
            process_function()
        else:
            logger.info(f'{json_filename}尚未到更新时间，跳过处理')

    # 使用统一函数处理各JSON文件
    process_json_file('astronauts.json', 24, process_astronauts)  # 每日更新一次
    process_json_file('iciba.json', 24, process_iciba)  # 每日更新一次
    process_json_file('news.json', 2, process_news)  # 每两小时更新一次
    logger.info('所有JSON文件处理完成！')

if __name__ == '__main__':
    main()