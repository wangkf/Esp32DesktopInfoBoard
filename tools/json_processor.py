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
# 天气API - 使用新的数据源
WEATHER_API_URL = "http://d1.weather.com.cn/weather_index/101110101.html"
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

## 1. 处理icba.json - 只保留指定字段：tts, content, note, fenxiang_img及result中的last_updated
def process_icba():
    file_path = os.path.join(data_dir, 'icba.json')
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
            
            logger.info(f'已成功处理icba.json，只保留指定字段，更新时间: {current_time}')
        else:
            logger.warning(f'没有获取到有效数据，不更新icba.json文件')
    except Exception as e:
        logger.error(f'处理icba.json时出错: {e}')

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

## 3. 处理weather.json - 从新的API获取天气信息
# 使用 http://d1.weather.com.cn/weather_index/101110101.html 作为数据源
# 设置iPhone用户代理以获取移动端数据
def process_weather():
    file_path = os.path.join(data_dir, 'weather.json')
    try:
        # 获取当前时间
        current_time = get_current_time()
        
        # 初始化数据结构
        data = {'result': {}}
        
        # 为天气API设置移动端User-Agent和Referer
        headers = {
            'User-Agent': 'Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1',
            'Referer': 'http://www.weather.com.cn/'
        }
        
        # 添加时间戳参数到URL，防止缓存
        timestamp = int(datetime.now().timestamp())
        weather_url_with_timestamp = f"{WEATHER_API_URL}?_={timestamp}"
        
        # 尝试从新API获取数据
        logger.info(f'从新天气API获取数据: {weather_url_with_timestamp}')
        response = http_get_request(weather_url_with_timestamp, headers=headers)
        
        # 打印响应状态码和部分内容用于调试
        if response:
            logger.info(f'天气API响应状态码: {response.status_code}')
            logger.info(f'天气API响应内容前500字符: {response.text[:500]}')
        
        # 只有在API请求成功且包含有效数据时才更新文件
        if response:
            # 解析HTML响应中的JSON数据
            # 尝试使用正确的编码格式解码响应内容
            # 首先尝试UTF-8，如果失败再尝试GB2312（中国天气网常用编码）
            try:
                content = response.content.decode('utf-8')
            except UnicodeDecodeError:
                content = response.content.decode('gb2312')
            
            # 使用正则表达式提取天气数据
            # 这个API返回的是JavaScript变量赋值形式的内容，需要提取其中的JSON数据
            try:
                # 匹配var cityDZ的部分，包含详细的天气数据
                city_dz_match = re.search(r'var\s+cityDZ\s*=\s*(.+?);', content, re.DOTALL)
                # 匹配var dataSK的部分，包含实时天气数据
                data_sk_match = re.search(r'var\s+dataSK\s*=\s*(.+?);', content, re.DOTALL)
                # 匹配var dataZS的部分，包含生活指数数据
                data_zs_match = re.search(r'var\s+dataZS\s*=\s*(.+?);', content, re.DOTALL)
                # 匹配var fc的部分，包含未来天气预报数据
                fc_match = re.search(r'var\s+fc\s*=\s*(.+?);', content, re.DOTALL)
                
                # 至少需要dataSK数据才能继续处理
                if data_sk_match:
                    # 解析JSON数据
                    data_sk_data = json.loads(data_sk_match.group(1))
                    city_dz_data = json.loads(city_dz_match.group(1)) if city_dz_match else {}
                    data_zs_data = json.loads(data_zs_match.group(1)) if data_zs_match else {}
                    # 打印data_zs_data的内容，用于调试
                    if data_zs_match:
                        logger.info(f'data_zs_data的完整内容: {data_zs_data}')
                    fc_data = json.loads(fc_match.group(1)) if fc_match else {}
                    
                    # 创建一个包含所有原始天气数据的字典
                    raw_weather_data = {
                        'cityDZ': city_dz_data,
                        'dataSK': data_sk_data,
                        'dataZS': data_zs_data,
                        'fc': fc_data,
                        'extracted_from': weather_url_with_timestamp,
                        'extraction_time': current_time
                    }
                    
                    # 保存原始天气数据到单独文件
                    raw_weather_file = os.path.join(data_dir, 'weather_raw.json')
                    try:
                        with open(raw_weather_file, 'w', encoding='utf-8') as f:
                            json.dump(raw_weather_data, f, ensure_ascii=False, indent=2)
                        logger.info(f'已成功保存原始天气数据到weather_raw.json')
                    except Exception as e:
                        logger.error(f'保存原始天气数据时出错: {e}')
                    
                    # 从cityDZ中获取weatherinfo
                    weather_info_data = city_dz_data.get('weatherinfo', {})
                    
                    # 构建符合原有格式的数据结构，并添加所有能获取的信息
                    result = {
                        'code': 200,
                        'msg': 'success',
                        'result': {
                            # 基础城市信息
                            'city': '西安',  # 直接使用城市名称，避免乱码问题
                            'city_code': city_dz_data.get('weatherinfo', {}).get('city', ''),
                            'city_name_en': data_sk_data.get('cityname', ''),
                            
                            # 实时天气信息 - 从dataSK中提取所有可能的字段
                            'realtime': {
                                'temperature': data_sk_data.get('temp', weather_info_data.get('temp', '')),
                                'temperature_f': data_sk_data.get('tempf', ''),
                                'humidity': data_sk_data.get('SD', '').replace('%%', '%'),
                                'humidity_alt': data_sk_data.get('sd', ''),
                                'info': weather_info_data.get('weather', ''),
                                'wid': weather_info_data.get('weathercode', ''),
                                'direct': data_sk_data.get('WD', ''),
                                'direct_en': data_sk_data.get('wde', ''),
                                'power': data_sk_data.get('WS', ''),
                                'power_detail': data_sk_data.get('wse', ''),
                                'pressure': data_sk_data.get('qy', ''),
                                'visibility': data_sk_data.get('njd', ''),
                                'obs_time': data_sk_data.get('time', ''),
                                'rain': data_sk_data.get('rain', ''),
                                'rain_24h': data_sk_data.get('rain24h', ''),
                                'aqi': data_sk_data.get('aqi', ''),
                                'pm25': data_sk_data.get('aqi_pm25', '')
                            },
                            
                            # 扩展的天气信息 - 从cityDZ的weatherinfo中提取
                            'extended_info': {
                                'temp_high': weather_info_data.get('temp', ''),
                                'temp_low': weather_info_data.get('tempn', ''),
                                'weather_night': weather_info_data.get('weathercoden', ''),
                                'wind_direction': weather_info_data.get('wd', ''),
                                'wind_scale': weather_info_data.get('ws', ''),
                                'forecast_time': weather_info_data.get('fctime', '')
                            },
                            
                            # 原始数据保留 - 用于调试和未来扩展
                            'raw_data': {
                                'city_dz_keys': list(city_dz_data.keys()),
                                'data_sk_keys': list(data_sk_data.keys()),
                                'data_zs_keys': list(data_zs_data.keys()),
                                'fc_keys': list(fc_data.keys())
                            },
                            
                            # 生活指数数据 - 从dataZS中提取
                            'life_index': {
                                # 根据实际data_zs_data结构映射生活指数
                                'comfort': data_zs_data.get('mf_hint', ''),  # 美发指数作为舒适度
                                'comfort_desc': data_zs_data.get('mf_des_s', ''),
                                'uv': data_zs_data.get('fs_hint', ''),  # 防晒指数作为紫外线指数
                                'uv_desc': data_zs_data.get('fs_des_s', ''),
                                'dressing': data_zs_data.get('pp_hint', ''),  # 化妆指数作为穿衣指数
                                'dressing_desc': data_zs_data.get('pp_des_s', ''),
                                'sport': data_zs_data.get('mf_hint', ''),  # 复用美发指数作为运动指数
                                'sport_desc': data_zs_data.get('mf_des_s', ''),
                                'car_wash': data_zs_data.get('ys_hint', ''),  # 雨伞指数作为洗车指数
                                'car_wash_desc': data_zs_data.get('ys_des_s', ''),
                                'cold': data_zs_data.get('zs_hint', ''),  # 中暑指数作为感冒指数
                                'cold_desc': data_zs_data.get('zs_des_s', ''),
                                'travel': data_zs_data.get('gz_hint', ''),  # 干燥指数作为旅游指数
                                'travel_desc': data_zs_data.get('gz_des_s', '')
                            },
                            
                            'future': []
                        }
                    }
                    
                    # 添加未来几天的天气预报
                    # 1. 首先尝试从fc数据中提取（如果有）
                    if fc_data:
                        logger.info(f'从fc数据中提取未来天气预报')
                        # 检查fc数据的结构并提取预报信息
                        if isinstance(fc_data, list):
                            # 直接遍历列表格式的fc数据
                            for forecast in fc_data:
                                if isinstance(forecast, dict):
                                    date = forecast.get('date', '')
                                    temp_high = forecast.get('temp', '')
                                    temp_low = forecast.get('tempn', '') or forecast.get('temp_min', '')
                                    weather = forecast.get('weather', '')
                                    direct = forecast.get('wd', '') or forecast.get('wind_direction', '')
                                    
                                    # 处理异常温度值
                                    if temp_high == '999':
                                        temp_high = data_sk_data.get('temp', '')
                                    
                                    if date:
                                        result['result']['future'].append({
                                            'date': date,
                                            'temperature': f"{temp_low}~{temp_high}",
                                            'weather': weather,
                                            'direct': direct
                                        })
                        elif isinstance(fc_data, dict):
                            # 处理字典格式的fc数据，根据结构特点提取信息
                            for key, value in fc_data.items():
                                if isinstance(value, dict):
                                    date = value.get('date', '')
                                    temp_high = value.get('temp', '')
                                    temp_low = value.get('tempn', '') or value.get('temp_min', '')
                                    weather = value.get('weather', '')
                                    direct = value.get('wd', '') or value.get('wind_direction', '')
                                    
                                    # 处理异常温度值
                                    if temp_high == '999':
                                        temp_high = data_sk_data.get('temp', '')
                                    
                                    if date:
                                        result['result']['future'].append({
                                            'date': date,
                                            'temperature': f"{temp_low}~{temp_high}",
                                            'weather': weather,
                                            'direct': direct
                                        })
                    
                    # 2. 如果fc数据中没有预报，尝试从city_dz_data的weatherinfo结构中提取
                    if not result['result']['future'] and 'weatherinfo' in city_dz_data and isinstance(city_dz_data['weatherinfo'], dict):
                        logger.info(f'从city_dz_data中提取今天的天气预报')
                        # 添加今天的天气信息作为第一个预报
                        # 处理异常温度值（如999）
                        temp_high = weather_info_data.get('temp', '')
                        if temp_high == '999':
                            temp_high = data_sk_data.get('temp', '')  # 使用实时温度作为最高温度
                            
                        today_forecast = {
                            'date': current_time.split(' ')[0],  # 使用当前日期
                            'temperature': f"{weather_info_data.get('tempn', '')}~{temp_high}",
                            'weather': weather_info_data.get('weather', ''),
                            'direct': weather_info_data.get('wd', '')
                        }
                        result['result']['future'].append(today_forecast)
                        logger.info(f'添加今天的天气预报: {today_forecast}')
                    
                    # 记录提取的数据结构信息用于调试
                    logger.info(f'提取的数据统计 - dataSK={bool(data_sk_data)}, cityDZ={bool(city_dz_data)}, dataZS={bool(data_zs_data)}, fc={bool(fc_data)}')
                    
                    # 如果没有获取到未来预报，添加空数组以保持数据结构完整
                    if not result['result']['future']:
                        logger.info(f'未找到详细的未来天气预报数据')
                    
                    # 添加更新时间
                    result['result']['last_updated'] = current_time
                    
                    # 保存文件
                    with open(file_path, 'w', encoding='utf-8') as f:
                        json.dump(result, f, ensure_ascii=False, indent=2)
                    
                    logger.info(f'已成功处理weather.json，使用新API获取数据，更新时间: {current_time}')
                else:
                    logger.error(f'无法从天气API响应中提取数据')
                    # 如果API数据解析失败，检查文件是否存在
                    if os.path.exists(file_path):
                        # 文件存在时只更新时间戳
                        with open(file_path, 'r', encoding='utf-8') as f:
                            data = json.load(f)
                        
                        # 确保数据结构正确
                        if 'result' not in data:
                            data['result'] = {}
                        
                        # 更新时间戳
                        data['result']['last_updated'] = current_time
                        
                        with open(file_path, 'w', encoding='utf-8') as f:
                            json.dump(data, f, ensure_ascii=False, indent=2)
                        
                        logger.warning(f'天气数据解析失败，使用现有数据更新weather.json时间戳: {current_time}')
                    else:
                        logger.warning(f'天气数据解析失败，且weather.json文件不存在，不创建新文件')
                
            except Exception as parse_error:
                logger.error(f'解析天气数据时出错: {parse_error}')
                # 如果解析出错，检查文件是否存在
                if os.path.exists(file_path):
                    # 文件存在时只更新时间戳
                    with open(file_path, 'r', encoding='utf-8') as f:
                        data = json.load(f)
                    
                    # 确保数据结构正确
                    if 'result' not in data:
                        data['result'] = {}
                    
                    # 更新时间戳
                    data['result']['last_updated'] = current_time
                    
                    with open(file_path, 'w', encoding='utf-8') as f:
                        json.dump(data, f, ensure_ascii=False, indent=2)
                    
                    logger.warning(f'天气数据解析失败，使用现有数据更新weather.json时间戳: {current_time}')
                else:
                    logger.warning(f'天气数据解析失败，且weather.json文件不存在，不创建新文件')
        else:
            # 如果API请求失败，检查文件是否存在
            if os.path.exists(file_path):
                # 文件存在时只更新时间戳
                with open(file_path, 'r', encoding='utf-8') as f:
                    data = json.load(f)
                
                # 确保数据结构正确
                if 'result' not in data:
                    data['result'] = {}
                
                # 更新时间戳
                data['result']['last_updated'] = current_time
                
                with open(file_path, 'w', encoding='utf-8') as f:
                    json.dump(data, f, ensure_ascii=False, indent=2)
                
                logger.warning(f'API请求失败，使用现有数据更新weather.json时间戳: {current_time}')
            else:
                logger.warning(f'API请求失败，且weather.json文件不存在，不创建新文件')
    except Exception as e:
        logger.error(f'处理weather.json时出错: {e}')

## 4. 处理astronauts.json - 从API获取数据并添加更新时间
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

# 5. 处理aprs.json - 通过socket连接APRS服务器，获取实时数据
def process_aprs():
    """一次性处理APRS数据"""
    import socket
    import time
    
    file_path = os.path.join(data_dir, 'aprs.json')
    config_path = os.path.join(data_dir, 'config.json')
    
    try:
        # 获取当前时间
        current_time = get_current_time()
        
        # 初始化数据结构
        data = {'result': {'packets': [], 'total': 0}}
        
        # 读取现有的aprs.json文件（如果存在）
        if os.path.exists(file_path):
            with open(file_path, 'r', encoding='utf-8') as f:
                data = json.load(f)
            
            # 确保数据结构正确
            if 'result' not in data:
                data['result'] = {}
            if 'packets' not in data['result']:
                data['result']['packets'] = []
        
        # 读取config.json获取APRS配置
        aprs_config = {}
        if os.path.exists(config_path):
            with open(config_path, 'r', encoding='utf-8') as f:
                config_data = json.load(f)
                if 'aprs' in config_data:
                    aprs_config = config_data['aprs']
        
        # 检查必要的APRS配置是否存在
        required_configs = ['callsign', 'passcode', 'server', 'port']
        if not all(key in aprs_config for key in required_configs):
            logger.warning('APRS配置不完整，跳过socket连接')
            # 仍然处理现有的数据
            if data['result']['packets']:
                process_existing_aprs_packets(data, current_time, file_path)
            return
        
        # 从配置中获取参数
        callsign = aprs_config['callsign']
        passcode = aprs_config['passcode']
        server = aprs_config['server']
        port = aprs_config['port']
        max_packets = aprs_config.get('max_packets', 20)  # 默认保留20条记录
        
        # 尝试建立socket连接并获取数据
        try:
            # 创建socket连接
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(10)  # 设置超时时间
            logger.info(f'正在连接到APRS服务器: {server}:{port}')
            sock.connect((server, port))
            
            # 登录APRS服务器 - 使用位置过滤，获取东经108.8726，北纬34.2487周边10公里以内的数据
            login_cmd = f'user {callsign} pass {passcode} vers ESP32DesktopInfoBoard 1.0 filter r/34.2487/108.8726/10'
            sock.send((login_cmd + '\r\n').encode())
            logger.info(f'已发送APRS登录命令: {login_cmd}')
            
            # 接收并解析数据（持续30秒）
            start_time = time.time()
            received_packets = 0
            buffer = ''
            
            while time.time() - start_time < 30:  # 连接时间30秒
                try:
                    chunk = sock.recv(1024).decode('utf-8', errors='ignore')
                    if not chunk:
                        logger.debug('没有接收到数据，连接可能已关闭')
                        break
                    
                    buffer += chunk
                    lines = buffer.split('\n')
                    buffer = lines.pop()  # 保留不完整的行
                    
                    for line in lines:
                        line = line.strip()
                        logger.debug(f'接收到APRS原始行: {line}')
                        if line and not line.startswith('#') and ':' in line:
                            # 解析APRS数据包
                            packet_data = parse_aprs_packet(line)
                            if packet_data:
                                data['result']['packets'].append(packet_data)
                                received_packets += 1
                                logger.info(f'接收到APRS数据包: {packet_data["callsign"]}')
                except socket.timeout:
                    logger.debug('接收数据超时')
                    break
            
            # 关闭socket连接
            sock.close()
            logger.info(f'APRS连接已关闭，共接收{received_packets}个数据包')
            
        except Exception as socket_error:
            logger.error(f'APRS socket连接错误: {socket_error}')
        
        # 处理所有数据包（包括新接收的和现有的）
        process_existing_aprs_packets(data, current_time, file_path, max_packets)
        
    except Exception as e:
        logger.error(f'处理aprs.json时出错: {e}')
        
def process_aprs_continuous():
    """持续获取APRS数据，直到用户回车确认退出或收集到6个不同呼号的数据"""
    import socket
    import time
    import threading
    
    file_path = os.path.join(data_dir, 'aprs.json')
    config_path = os.path.join(data_dir, 'config.json')
    
    try:
        # 初始化数据结构
        data = {'result': {'packets': [], 'total': 0}}
        
        # 读取现有的aprs.json文件（如果存在）
        if os.path.exists(file_path):
            with open(file_path, 'r', encoding='utf-8') as f:
                data = json.load(f)
            
            # 确保数据结构正确
            if 'result' not in data:
                data['result'] = {}
            if 'packets' not in data['result']:
                data['result']['packets'] = []
        
        # 读取config.json获取APRS配置
        aprs_config = {}
        if os.path.exists(config_path):
            with open(config_path, 'r', encoding='utf-8') as f:
                config_data = json.load(f)
                if 'aprs' in config_data:
                    aprs_config = config_data['aprs']
        
        # 检查必要的APRS配置是否存在
        required_configs = ['callsign', 'passcode', 'server', 'port']
        if not all(key in aprs_config for key in required_configs):
            logger.warning('APRS配置不完整，无法进行持续数据收集')
            return
        
        # 从配置中获取参数
        callsign = aprs_config['callsign']
        passcode = aprs_config['passcode']
        server = aprs_config['server']
        port = aprs_config['port']
        max_unique_callsigns = 6  # 目标收集6个不同呼号
        
        # 创建一个集合来跟踪已收集的呼号
        unique_callsigns_collected = set()
        
        # 用于控制程序退出的标志
        should_exit = threading.Event()
        
        # 定义检查用户输入的线程函数
        def check_user_input():
            try:
                # 尝试使用msvcrt模块获取键盘输入（Windows系统）
                import msvcrt
                logger.info('按回车键退出APRS数据收集...')
                while not should_exit.is_set():
                    if msvcrt.kbhit():
                        key = msvcrt.getch()
                        if key == b'\r':  # 回车键
                            logger.info('检测到回车键，准备退出APRS数据收集...')
                            should_exit.set()
                            break
                    time.sleep(0.1)
            except ImportError:
                # 如果是Linux/Mac系统，使用input函数
                logger.info('按回车键退出APRS数据收集...')
                input()  # 等待用户按下回车键
                should_exit.set()
        
        # 启动用户输入检查线程
        input_thread = threading.Thread(target=check_user_input)
        input_thread.daemon = True
        input_thread.start()
        
        # 尝试建立socket连接并持续获取数据
        try:
            # 创建socket连接
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(1.0)  # 设置超时时间，以便定期检查退出标志
            logger.info(f'正在连接到APRS服务器: {server}:{port}')
            sock.connect((server, port))
            
            # 登录APRS服务器 - 使用位置过滤，获取东经108.8726，北纬34.2487周边10公里以内的数据
            login_cmd = f'user {callsign} pass {passcode} vers ESP32DesktopInfoBoard 1.0 filter r/34.2487/108.8726/10'
            sock.send((login_cmd + '\r\n').encode())
            logger.info(f'已发送APRS登录命令: {login_cmd}')
            
            # 持续接收并解析数据
            buffer = ''
            while not should_exit.is_set() and len(unique_callsigns_collected) < max_unique_callsigns:
                try:
                    chunk = sock.recv(1024).decode('utf-8', errors='ignore')
                    if not chunk:
                        logger.warning('APRS服务器连接已关闭')
                        break
                    
                    buffer += chunk
                    lines = buffer.split('\n')
                    buffer = lines.pop()  # 保留不完整的行
                    
                    for line in lines:
                        line = line.strip()
                        if line and not line.startswith('#') and ':' in line:
                            # 解析APRS数据包
                            packet_data = parse_aprs_packet(line)
                            if packet_data:
                                callsign = packet_data.get('callsign')
                                # 添加数据包
                                data['result']['packets'].append(packet_data)
                                
                                # 仅统计呼号首字母为B的记录
                                if callsign and callsign[0].upper() == 'B' and callsign not in unique_callsigns_collected:
                                    unique_callsigns_collected.add(callsign)
                                    logger.info(f'发现新的B开头呼号: {callsign} (已收集: {len(unique_callsigns_collected)}/{max_unique_callsigns})')
                                
                                # 检查是否已收集足够的B开头呼号
                                if len(unique_callsigns_collected) >= max_unique_callsigns:
                                    logger.info(f'已收集{max_unique_callsigns}个不同B开头呼号的APRS数据，准备保存并退出...')
                                    should_exit.set()
                                    break
                except socket.timeout:
                    # 超时是正常的，继续循环检查退出标志
                    continue
                except Exception as e:
                    logger.error(f'接收APRS数据时出错: {e}')
                    break
            
            # 关闭socket连接
            sock.close()
            logger.info(f'APRS连接已关闭，共收集{len(unique_callsigns_collected)}个不同呼号的数据')
            
        except Exception as socket_error:
            logger.error(f'APRS socket连接错误: {socket_error}')
        
        # 处理并保存数据
        if data['result']['packets']:
            current_time = get_current_time()
            process_existing_aprs_packets(data, current_time, file_path, max_unique_callsigns)
        
    except Exception as e:
        logger.error(f'持续处理aprs.json时出错: {e}')


def parse_aprs_packet(packet_line):
    """解析APRS数据包行，提取有用信息"""
    import time
    try:
        # 示例APRS数据包格式: CALLSIGN>APRS,TCPIP*,qAC,CLIENT:!LAT/LON...
        # 分割头部和数据部分
        parts = packet_line.split(':', 1)
        if len(parts) < 2:
            return None
        
        header = parts[0]
        data = parts[1]
        
        # 提取呼号
        callsign_part = header.split('>')[0]
        
        # 获取当前时间
        current_time = get_current_time()
        
        # 构建数据包信息 - 只保留timestamp字段，移除parsed_time字段
        packet_info = {
            'callsign': callsign_part,
            'raw_data': packet_line,
            'timestamp': current_time
        }
        
        # 尝试提取位置信息（如果有）
        if data.startswith('!') or data.startswith('='):
            # 简单解析位置（完整的解析需要更复杂的逻辑）
            position_match = re.search(r'[!=](\d{4}\.\d{2})([NS])(\d{5}\.\d{2})([EW])', data)
            if position_match:
                lat = position_match.group(1)
                lat_dir = position_match.group(2)
                lon = position_match.group(3)
                lon_dir = position_match.group(4)
                
                # 简单格式化经纬度
                lat_deg = lat[:2] if len(lat) > 4 else lat[:1]
                lat_min = lat[2:] if len(lat) > 4 else lat[1:]
                lon_deg = lon[:3] if len(lon) > 5 else lon[:2]
                lon_min = lon[3:] if len(lon) > 5 else lon[2:]
                
                packet_info['location'] = {
                    'latitude': f'{lat_deg}°{lat_min}\'{lat_dir}',
                    'longitude': f'{lon_deg}°{lon_min}\'{lon_dir}'
                }
        
        return packet_info
    except Exception as e:
        logger.error(f'解析APRS数据包出错: {e}')
        return None


def process_existing_aprs_packets(data, current_time, file_path, max_packets=20):
    """处理现有的APRS数据包，排序和去重，仅保留呼号首字母为B的数据"""
    try:
        # 获取所有数据包
        packets = data['result']['packets']
        
        # 按时间从近到远排序
        def get_timestamp(packet):
            try:
                return datetime.strptime(packet['timestamp'], '%Y-%m-%d %H:%M:%S')
            except:
                # 如果时间格式不正确，返回一个很早的时间
                return datetime.min
        
        sorted_packets = sorted(packets, key=get_timestamp, reverse=True)
        
        # 保留指定数量的呼号不重合的记录，并且呼号首字母为B
        unique_callsigns = set()
        filtered_packets = []
        
        for packet in sorted_packets:
            callsign = packet.get('callsign')
            # 添加呼号首字母为B的过滤条件
            if callsign and callsign[0].upper() == 'B' and callsign not in unique_callsigns:
                filtered_packets.append(packet)
                unique_callsigns.add(callsign)
                if len(filtered_packets) >= max_packets:
                    break
        
        # 更新数据
        data['result']['packets'] = filtered_packets
        
        # 更新总数和时间戳
        data['result']['total'] = len(data['result']['packets'])
        data['result']['last_update'] = current_time
        
        # 保存文件
        with open(file_path, 'w', encoding='utf-8') as f:
            json.dump(data, f, ensure_ascii=False, indent=2)
        
        logger.info(f'已成功处理aprs.json，保留了{len(data["result"]["packets"])}条呼号首字母为B且不重合的记录')
    except Exception as e:
        logger.error(f'处理现有APRS数据包时出错: {e}')

## 6. 从config.h提取配置并更新config.json
def update_config_from_header():
    file_path = os.path.join(data_dir, 'config.json')
    config_h_path = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'src', 'config.h')
    
    try:
        # 读取并解析config.h文件
        if not os.path.exists(config_h_path):
            logger.error(f'config.h文件不存在: {config_h_path}')
            return
        
        # 初始化配置字典
        config_data = {
            'api': {},
            'display': {},
            'weather': {},
            'wifi': {},
            'ntp': {},
            'hardware': {},
            'button': {},
            'aprs': {},
            'other': {}
        }
        
        # 读取config.h文件内容
        with open(config_h_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
        
        # 解析每一行，提取#define和const定义的配置项
        for line in lines:
            line = line.strip()
            
            # 提取#define配置
            if line.startswith('#define') and '//' in line:
                parts = line.split('//')[0].split()
                if len(parts) >= 3:
                    key = parts[1]
                    value = ' '.join(parts[2:])
                    
                    # 去除引号
                    if value.startswith('"') and value.endswith('"'):
                        value = value[1:-1]
                    
                    # 按类型分类
                    if key == 'API_KEY':
                        config_data['api']['key'] = value
                    elif key == 'WIFI_SSID':
                        config_data['wifi']['ssid'] = value
                    elif key == 'WIFI_PASSWORD':
                        config_data['wifi']['password'] = value
                    elif key == 'NTP_SERVER':
                        config_data['ntp']['server'] = value
                    elif key == 'NTP_TIMEZONE':
                        config_data['ntp']['timezone'] = int(value)
                    elif key == 'DATA_CACHE_INTERVAL':
                        config_data['other']['data_cache_interval'] = int(value)
                    elif key == 'APRS_CALLSIGN':
                        config_data['aprs']['callsign'] = value
                    elif key == 'APRS_PASSCODE':
                        config_data['aprs']['passcode'] = value
                    elif key == 'APRS_SERVER':
                        config_data['aprs']['server'] = value
                    elif key == 'APRS_PORT':
                        config_data['aprs']['port'] = int(value)
                    elif key == 'APRS_RANGE_KM':
                        config_data['aprs']['range_km'] = int(value)
                    elif key == 'APRS_MAX_PACKETS':
                        config_data['aprs']['max_packets'] = int(value)
            
            # 提取const配置
            elif line.startswith('const') and ('=' in line) and not line.endswith(';'):
                # 处理带注释的行
                if '//' in line:
                    line = line.split('//')[0]
                
                parts = line.split('=')
                if len(parts) == 2:
                    left_part = parts[0].strip()
                    value = parts[1].strip().rstrip(';')
                    
                    # 提取变量名
                    var_parts = left_part.split()
                    if len(var_parts) >= 3:
                        key = var_parts[2]
                        
                        # 去除引号
                        if value.startswith('"') and value.endswith('"'):
                            value = value[1:-1]
                        # 尝试转换为数字
                        elif value.replace('.', '', 1).isdigit():
                            if '.' in value:
                                value = float(value)
                            else:
                                value = int(value)
                        
                        # 按类型分类
                        if key == 'screenWidth' or key == 'screenHeight':
                            config_data['display'][key] = value
                        elif key == 'BUTTON_PIN' or key == 'LIGHT_SENSOR_PIN' or key == 'SCREEN_BRIGHTNESS_PIN':
                            config_data['hardware'][key] = value
                        elif key == 'DEBOUNCE_DELAY' or key == 'SHORT_PRESS_THRESHOLD' or key == 'LONG_PRESS_THRESHOLD' or key == 'MULTI_CLICK_THRESHOLD':
                            config_data['button'][key] = value
                        elif key == 'AUTO_SCREEN_SWITCH_INTERVAL' or key == 'updateInterval' or key == 'brightnessUpdateInterval':
                            config_data['other'][key] = value
                        elif key == 'DARK_THRESHOLD' or key == 'VERY_DARK_THRESHOLD':
                            config_data['other'][key] = value
        
        # 如果文件已存在，先读取现有数据
        if os.path.exists(file_path):
            with open(file_path, 'r', encoding='utf-8') as f:
                existing_data = json.load(f)
            
            # 合并现有数据和新提取的数据
            for category, items in config_data.items():
                if category not in existing_data:
                    existing_data[category] = {}
                for key, value in items.items():
                    existing_data[category][key] = value
            
            config_data = existing_data
        
        # 保存更新后的配置文件
        with open(file_path, 'w', encoding='utf-8') as f:
            json.dump(config_data, f, ensure_ascii=False, indent=2)
        
        logger.info('已成功从config.h提取配置并更新config.json')
    except Exception as e:
        logger.error(f'处理config.json时出错: {e}')

# 重命名原函数以保持兼容性
check_config = update_config_from_header

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
    process_json_file('weather.json', 0.5, process_weather)  # 半小时内不更新
    process_json_file('icba.json', 24, process_icba)  # 每日更新一次
    process_json_file('news.json', 2, process_news)  # 每两小时更新一次
    
    # 检查配置文件
    check_config()
    
    # APRS处理放到最后，持续获取数据直到用户退出或收集到6个不同呼号的数据
    logger.info('开始APRS数据收集，按回车键退出...')
    process_aprs_continuous()
    
    logger.info('所有JSON文件处理完成！')

if __name__ == '__main__':
    main()