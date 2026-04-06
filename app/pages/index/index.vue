<template>
	<view class="wrap">
		<view class="dev-area">
			<!-- 温度卡片（添加了阈值控制） -->
			        <view class="dev-cart2">
			            <view class="">
			                <view class="dev-name">温度</view>
			                <image class="dev-logo" src="../../static/temp.png" mode=""></image>
			            </view>
			            <view class="dev-data">{{temp}}℃</view>
			            
			            <!-- 温度阈值控制组件 -->
			            <view class="threshold-control">
			                <view class="threshold-labels">
			                    <text>0℃</text>
			                    <text class="threshold-value">{{tempThreshold}}℃</text>
			                    <text>100℃</text>
			                </view>
			                <slider 
			                    class="threshold-slider"
			                    :value="tempThreshold"
			                    min="0" 
			                    max="100" 
			                    step="1"
			                    activeColor="#409EFF"
			                    backgroundColor="#e5e5e5"
			                    block-color="#409EFF"
			                    block-size="20"
			                    @change="onThresholdChange"
			                    show-value
			                />
			            </view>
			        </view>
			        
			        <!-- 其他卡片保持不变 -->
			<!-- <view class="dev-cart">
				<view class="">
					<view class="dev-name">温度</view>
					<image class="dev-logo" src="../../static/temp.png" mode=""></image>
				</view>
				<view class="dev-data">{{temp}} ℃</view>
			</view> -->
			
			<view class="dev-cart">
				<view class="">
					<view class="dev-name">湿度</view>
					<image class="dev-logo" src="../../static/humi.png" mode=""></image>
				</view>
				<view class="dev-data">{{humi}} %</view>
			</view>
			<view class="dev-cart">
				<view class="">
					<view class="dev-name">空气质量</view>
					<image class="dev-logo" src="../../static/aqi.png" mode=""></image>
				</view>
				<view class="dev-data">{{aqi}}</view>
			</view>
			<view class="dev-cart2">
				<view class="">
					<view class="dev-name">光照强度</view>
					<image class="dev-logo" src="../../static/ill.png" mode=""></image>
				</view>
				<view class="dev-data">{{ill}} lux</view>
				
				<view class="threshold-control">
				    <view class="threshold-labels">
				        <text>0lux</text>
				        <text class="threshold-value">{{ill_LL}}lux</text>
				        <text>1000lux</text>
				    </view>
				    <slider 
				        class="threshold-slider"
				        :value="ill_LL" 
				        min="0" 
				        max="1000" 
				        step="1"
				        activeColor="#409EFF"
				        backgroundColor="#e5e5e5"
				        block-color="#409EFF"
				        block-size="20"
				        @change="onThresholdChange1"
				        show-value
				    />
				</view>
			</view>
			
			
			

		</view>
		<view class="dev-area1">
			<view class="dev-cart1">
				<view class="">
					<view class="dev-name">照明灯</view>
					<image class="dev-logo" src="../../static/light.png" mode=""></image>
				</view>
				<switch :checked="led" @change="onLedSwitch" color="#2b9939" />
			</view>
		</view>
	</view>

</template>

<script>
	const {
		createCommonToken
	} = require('@/key.js')
	export default {
		data() {
			return {
				temp: '',
				humi: '',
				ill: '',
				aqi: '',
				led: true,
				tempThreshold: 50, // 温度阈值（默认值）
				ill_LL: 100,
				token: '',
			}
		},
		onLoad() {
			const params = {
				author_key: 'lXjeiJaAB5OKPGBFscJ8qOaXPIrgDBbNdoP0KeBIi3nLbSIO/QAfx8/21m593lgN',
				version: '2022-05-01',
				user_id: '448173',
			}
			this.token = createCommonToken(params);
		},
		onShow() {
			this.fetchDevData();
			setInterval(() => {
				this.fetchDevData();
			}, 2000)
		},
		methods: {
			fetchDevData() {															//获取数据函数
				uni.request({															//发起请求
					url: 'https://iot-api.heclouds.com/thingmodel/query-device-property', //ONENET接口地址。
					method: 'GET',															//请求方式GET
					data: {
						product_id: 'ch40Qpq8G4',
						device_name: 'test'
					},
					header: {
						'authorization': this.token //自定义请求头信息
					},
					success: (res) => {
						console.log(res.data);
						this.temp = res.data.data[4].value;
						this.humi = res.data.data[3].value;
						this.ill = res.data.data[6].value;
						this.aqi = res.data.data[10].value;
						this.tempThreshold=res.data.data[12].value;
						this.ill_LL = res.data.data[11].value;
						this.led = res.data.data[0].value === 'true';

					}
				});
			},
			onLedSwitch(event) {															//下发数据
				this.led = event.detail.value;
				console.log("照明灯状态更新为:", this.led);
				uni.request({
					url: 'https://iot-api.heclouds.com/thingmodel/set-device-property', //ONENET接口地址。
					method: 'POST',															//请求方式POST	
					data: {
						product_id: 'ch40Qpq8G4',
						device_name: 'test',
						params: {
							"CMD": this.led
						}
					},
					header: {
						'authorization': this.token //自定义请求头信息
					},
					success: (res) => {
					    console.log('照明灯状态更新成功:', res);
					    uni.showToast({
					        title: '照明灯状态设置成功',
					        icon: 'success'
					    });
					},
					fail: (err) => {
					    console.error('照明灯状态更新失败:', err);
					    uni.showToast({
					        title: '照明灯状态设置失败',
					        icon: 'error'
					    });
					}
				});
			},
			onThresholdChange(e) {
			    // 更新阈值
			    this.tempThreshold = e.detail.value;
			    console.log("温度阈值更新为:", this.tempThreshold + "℃");
			    
			    // 发送阈值到OneNET平台
			    uni.request({
			        url: 'https://iot-api.heclouds.com/thingmodel/set-device-property',
			        method: 'POST',
			        header: {
			            'authorization': this.token // 添加认证token
			        },
			        data: { 
			            product_id: 'ch40Qpq8G4',
			            device_name: 'test',
			            params: {
			                "tempThreshold": this.tempThreshold // 使用当前阈值值
			            }
			        },
			        success: (res) => {
			            console.log('阈值更新成功:', res);
			            uni.showToast({
			                title: '阈值设置成功',
			                icon: 'success'
			            });
			        },
			        fail: (err) => {
			            console.error('阈值更新失败:', err);
			            uni.showToast({
			                title: '设置失败',
			                icon: 'error'
			            });
			        }
			    });
			},
			onThresholdChange1(e1) {
			    // 更新阈值
			    this.ill_LL = e1.detail.value;
			    console.log("光照阈值更新为:", this.ill_LL + "%");
			    
			    // 发送阈值到OneNET平台
			    uni.request({
			        url: 'https://iot-api.heclouds.com/thingmodel/set-device-property',
			        method: 'POST',
			        header: {
			            'authorization': this.token // 添加认证token
			        },
			        data: { 
			            product_id: 'ch40Qpq8G4',
			            device_name: 'test',
			            params: {
			                "lightThreshold": this.ill_LL // 使用当前阈值值
			            }
			        },
			        success: (res) => {
			            console.log('阈值更新成功:', res);
			            uni.showToast({
			                title: '阈值设置成功',
			                icon: 'success'
			            });
			        },
			        fail: (err) => {
			            console.error('阈值更新失败:', err);
			            uni.showToast({
			                title: '设置失败',
			                icon: 'error'
			            });
			        }
			    });
			}


		}
	}
</script>

<style>
	/* 阈值控制区域 */
	.threshold-control {
	 margin-top: 15px;
	    width: 60%;
	    padding: 0 10px;
	}
	
	.threshold-labels {
	    display: flex;
	    justify-content: space-between;
	    margin-bottom: 5px;
	    font-size: 12px;
	    color: #666;
	}
	
	.threshold-value {
	    font-weight: bold;
	    color: #409EFF;
	}
	
	/* 滑动条样式 */
	.threshold-slider {
	    width: 100%;
	}
	
	.wrap {
		padding: 50rpx;
	}

	.dev-area {
		display: flex;
		justify-content: space-between;
		flex-wrap: wrap;
	}
	.dev-area1 {
		display: flex;
		justify-content: center;
		flex-wrap: wrap;
	}
	.dev-cart {
		height: 150rpx;
		width: 320rpx;
		border-radius: 30rpx;
		margin-top: 30rpx;
		display: flex;
		justify-content: space-around;
		align-items: center;
		box-shadow: 0 0 15rpx #ccc;
	}
	.dev-cart1 {
		height: 150rpx;
		width: 640rpx;
		border-radius: 30rpx;
		margin-top: 200rpx;
		display: flex;
		justify-content: space-around;
		align-items: center;
		box-shadow: 0 0 15rpx #ccc;
	}
	.dev-cart2 {
		height: 150rpx;
		width: 100%;
		border-radius: 30rpx;
		margin-top: 30rpx;
		/* margin-top: 80%; */
		display: flex;
		justify-content: space-around;
		align-items: center;
		box-shadow: 0 0 15rpx #ccc;
	}
	.dev-name {
		font-size: 20rpx;
		text-align: center;
		color: #6d6d6d;
	}

	.dev-logo {
		width: 70rpx;
		height: 70rpx;
		margin-top: 10rpx;
	}

	.dev-data {
		font-size: 50rpx;
		margin-left: 20rpx;
		color: #6d6d6d;
	}
	
	.dev-me {
		font-size: 40rpx;
		color: #6d6d6d;
	}
</style>