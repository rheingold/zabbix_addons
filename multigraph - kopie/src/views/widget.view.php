<?php declare(strict_types = 0);
/**
 * Multigraph widget view
 * 
 * @var CView $this
 * @var array $data
 */

// Display error if present
if (!empty($data['error'])) {
	$body = (new CDiv([
		(new CDiv([
			new CTag('h4', true, $data['name'])
		]))->addClass('multigraph-widget-header'),
		(new CDiv([
			(new CTag('p', true, $data['error']))->addClass('error-message')
		]))->addClass('multigraph-widget-body')
	]))->addClass('multigraph-widget');

	(new CWidgetView($data))
		->addItem($body)
		->show();
	
	return;
}

// Check if we actually have graph data with series
if (empty($data['graph_data']) || empty($data['graph_data']['series'])) {
	$body = (new CDiv([
		(new CDiv([
			new CTag('h4', true, $data['name'])
		]))->addClass('multigraph-widget-header'),
		(new CDiv([
			new CTag('p', true, 'No graph data available.')
		]))->addClass('multigraph-widget-body')
	]))->addClass('multigraph-widget');

	(new CWidgetView($data))
		->addItem($body)
		->show();
	
	return;
}

// Render graph if data available
$graph_id = 'multigraph_' . substr(md5(uniqid('', true)), 0, 8);

$body = (new CDiv([
	(new CDiv([
		(new CTag('canvas'))
			->setId($graph_id . '_canvas')
			->addStyle('position: absolute; top: 0; left: 0; width: 100%; height: 100%;'),
		(new CTag('canvas'))
			->setId($graph_id . '_overlay')
			->addStyle('position: absolute; top: 0; left: 0; width: 100%; height: 100%;')
	]))
		->setId($graph_id)
		->addClass('multigraph-container')
		->addStyle('position: relative; width: 100%; height: 100%;')
]))->addClass('multigraph-widget')
	->addStyle('height: 100%;');

(new CWidgetView($data))
	->addItem($body)
	->setVar('graph_data', $data['graph_data'])
	->show();

return;

// OLD INLINE JS CODE BELOW - NOT USED
$graph_js = '
(function() {
	try {
		const graphId = ' . json_encode($graph_id) . ';
		const graphData = ' . json_encode($data['graph_data']) . ';
		
		const container = document.getElementById(graphId);
		if (!container) {
			console.error("Multigraph: Container not found:", graphId);
			return;
		}
		
		if (!graphData || !graphData.series || graphData.series.length === 0) {
			container.innerHTML = "<p style=\'padding: 20px;\'>No series data available</p>";
			return;
		}
		
		container.style.position = "relative";
		container.innerHTML = "";
		
		const canvas = document.createElement("canvas");
		canvas.width = container.offsetWidth || 600;
		canvas.height = container.offsetHeight || 400;
		canvas.style.display = "block";
		container.appendChild(canvas);
		
		const ctx = canvas.getContext("2d");
		
		let minValue = Infinity, maxValue = -Infinity;
		let minTime = Infinity, maxTime = -Infinity;
		
		graphData.series.forEach(series => {
			series.data.forEach(point => {
				minTime = Math.min(minTime, point[0]);
				maxTime = Math.max(maxTime, point[0]);
				minValue = Math.min(minValue, point[1]);
				maxValue = Math.max(maxValue, point[1]);
			});
		});
		
		if (graphData.y_min !== "auto" && !isNaN(parseFloat(graphData.y_min))) {
			minValue = parseFloat(graphData.y_min);
		} else {
			const valuePadding = (maxValue - minValue) * 0.1;
			minValue -= valuePadding;
		}
		
		if (graphData.y_max !== "auto" && !isNaN(parseFloat(graphData.y_max))) {
			maxValue = parseFloat(graphData.y_max);
		} else {
			const valuePadding = (maxValue - minValue) * 0.1;
			maxValue += valuePadding;
		}
		
		let margin = { top: 30, right: 20, bottom: 50, left: 70 };
		
		if (graphData.show_legend) {
			switch(graphData.legend_position) {
				case 1: margin.right = 150; break;
				case 2: margin.bottom = 80 + (graphData.series.length * 20); break;
			}
		}
		
		const plotWidth = canvas.width - margin.left - margin.right;
		const plotHeight = canvas.height - margin.top - margin.bottom;
		
		const xScale = (timestamp) => margin.left + ((timestamp - minTime) / (maxTime - minTime)) * plotWidth;
		const yScale = (value) => canvas.height - margin.bottom - ((value - minValue) / (maxValue - minValue)) * plotHeight;
		
		ctx.fillStyle = "#ffffff";
		ctx.fillRect(0, 0, canvas.width, canvas.height);
		
		if (graphData.show_grid) {
			ctx.strokeStyle = "#e0e0e0";
			ctx.lineWidth = 1;
			
			const gridLines = graphData.grid_density === "auto" ? 5 : parseInt(graphData.grid_density);
			
			for (let i = 0; i <= gridLines; i++) {
				const y = margin.top + (plotHeight / gridLines) * i;
				ctx.beginPath();
				ctx.moveTo(margin.left, y);
				ctx.lineTo(canvas.width - margin.right, y);
				ctx.stroke();
				
				const value = maxValue - ((maxValue - minValue) / gridLines) * i;
				ctx.fillStyle = "#666";
				ctx.font = "11px Arial";
				ctx.textAlign = "right";
				ctx.fillText(value.toFixed(2), margin.left - 5, y + 4);
			}
			
			for (let i = 0; i <= gridLines; i++) {
				const x = margin.left + (plotWidth / gridLines) * i;
				ctx.beginPath();
				ctx.moveTo(x, margin.top);
				ctx.lineTo(x, canvas.height - margin.bottom);
				ctx.stroke();
				
				const timestamp = minTime + ((maxTime - minTime) / gridLines) * i;
				const date = new Date(timestamp * 1000);
				const timeStr = date.toLocaleTimeString("en-US", {hour: "2-digit", minute: "2-digit"});
				ctx.fillStyle = "#666";
				ctx.textAlign = "center";
				ctx.fillText(timeStr, x, canvas.height - margin.bottom + 20);
			}
		}
		
		ctx.strokeStyle = "#333";
		ctx.lineWidth = 2;
		ctx.beginPath();
		ctx.moveTo(margin.left, margin.top);
		ctx.lineTo(margin.left, canvas.height - margin.bottom);
		ctx.lineTo(canvas.width - margin.right, canvas.height - margin.bottom);
		ctx.stroke();
		
		if (graphData.y_axis_label) {
			ctx.save();
			ctx.translate(15, canvas.height / 2);
			ctx.rotate(-Math.PI / 2);
			ctx.fillStyle = "#333";
			ctx.font = "bold 12px Arial";
			ctx.textAlign = "center";
			ctx.fillText(graphData.y_axis_label, 0, 0);
			ctx.restore();
		}
		
		graphData.series.forEach((series) => {
			if (series.data.length === 0) return;
			
			if (graphData.fill_opacity > 0) {
				const alpha = Math.round(graphData.fill_opacity * 255).toString(16).padStart(2, "0");
				ctx.fillStyle = series.color + alpha;
				ctx.beginPath();
				ctx.moveTo(xScale(series.data[0][0]), canvas.height - margin.bottom);
				series.data.forEach(point => {
					ctx.lineTo(xScale(point[0]), yScale(point[1]));
				});
				ctx.lineTo(xScale(series.data[series.data.length - 1][0]), canvas.height - margin.bottom);
				ctx.closePath();
				ctx.fill();
			}
			
			ctx.strokeStyle = series.color;
			ctx.lineWidth = 2;
			ctx.beginPath();
			series.data.forEach((point, i) => {
				const x = xScale(point[0]);
				const y = yScale(point[1]);
				if (i === 0) ctx.moveTo(x, y);
				else ctx.lineTo(x, y);
			});
			ctx.stroke();
		});
		
		if (graphData.show_legend) {
			ctx.font = "12px Arial";
			
			switch(graphData.legend_position) {
				case 0:
					let legendY = margin.top + 10;
					graphData.series.forEach(series => {
						ctx.fillStyle = series.color;
						ctx.fillRect(margin.left + 10, legendY, 12, 12);
						ctx.fillStyle = "#000";
						ctx.textAlign = "left";
						ctx.fillText(series.name, margin.left + 27, legendY + 10);
						legendY += 20;
					});
					break;
					
				case 1:
					let rightY = margin.top + 10;
					graphData.series.forEach(series => {
						const legendX = canvas.width - margin.right + 10;
						ctx.fillStyle = series.color;
						ctx.fillRect(legendX, rightY, 12, 12);
						ctx.fillStyle = "#000";
						ctx.textAlign = "left";
						ctx.fillText(series.name, legendX + 17, rightY + 10);
						rightY += 20;
					});
					break;
					
				case 2:
					let bottomX = margin.left + 10;
					let bottomY = canvas.height - margin.bottom + 40;
					graphData.series.forEach(series => {
						ctx.fillStyle = series.color;
						ctx.fillRect(bottomX, bottomY, 12, 12);
						ctx.fillStyle = "#000";
						ctx.textAlign = "left";
						const textWidth = ctx.measureText(series.name).width;
						ctx.fillText(series.name, bottomX + 17, bottomY + 10);
						bottomX += textWidth + 35;
						if (bottomX > canvas.width - 200) {
							bottomX = margin.left + 10;
							bottomY += 20;
						}
					});
					break;
			}
		}
		
		const tooltip = document.createElement("div");
		tooltip.style.cssText = "position: fixed; background: rgba(0,0,0,0.85); color: white; padding: 10px; border-radius: 4px; font-size: 12px; pointer-events: none; display: none; z-index: 10000; white-space: nowrap;";
		document.body.appendChild(tooltip);
		
		const overlayCanvas = document.createElement("canvas");
		overlayCanvas.width = canvas.width;
		overlayCanvas.height = canvas.height;
		overlayCanvas.style.cssText = "position: absolute; top: 0; left: 0;";
		container.appendChild(overlayCanvas);
		
		const overlayCtx = overlayCanvas.getContext("2d");
		
		overlayCanvas.addEventListener("mousemove", (e) => {
			const rect = overlayCanvas.getBoundingClientRect();
			const mouseX = e.clientX - rect.left;
			const mouseY = e.clientY - rect.top;
			
			if (mouseX < margin.left || mouseX > canvas.width - margin.right ||
				mouseY < margin.top || mouseY > canvas.height - margin.bottom) {
				tooltip.style.display = "none";
				overlayCtx.clearRect(0, 0, overlayCanvas.width, overlayCanvas.height);
				return;
			}
			
			const hoveredTime = minTime + ((mouseX - margin.left) / plotWidth) * (maxTime - minTime);
			overlayCtx.clearRect(0, 0, overlayCanvas.width, overlayCanvas.height);
			
			let tooltipContent = "";
			const date = new Date(hoveredTime * 1000);
			tooltipContent += "<div style=\'border-bottom: 1px solid #666; margin-bottom: 4px; padding-bottom: 4px; font-weight: bold;\'>" + date.toLocaleString() + "</div>";
			
			graphData.series.forEach(series => {
				let nearestPoint = null;
				let nearestDist = Infinity;
				
				series.data.forEach(point => {
					const dist = Math.abs(point[0] - hoveredTime);
					if (dist < nearestDist) {
						nearestDist = dist;
						nearestPoint = point;
					}
				});
				
				if (nearestPoint) {
					const x = xScale(nearestPoint[0]);
					const y = yScale(nearestPoint[1]);
					
					overlayCtx.fillStyle = series.color;
					overlayCtx.beginPath();
					overlayCtx.arc(x, y, 5, 0, 2 * Math.PI);
					overlayCtx.fill();
					overlayCtx.strokeStyle = "#fff";
					overlayCtx.lineWidth = 2;
					overlayCtx.stroke();
					
					tooltipContent += "<div style=\'margin: 2px 0;\'><span style=\'color:" + series.color + "; font-size: 16px;\'>●</span> " + series.name + ": <strong>" + nearestPoint[1].toFixed(2) + "</strong></div>";
				}
			});
			
			tooltip.innerHTML = tooltipContent;
			tooltip.style.display = "block";
			tooltip.style.left = (e.clientX + 15) + "px";
			tooltip.style.top = (e.clientY + 15) + "px";
		});
		
		overlayCanvas.addEventListener("mouseleave", () => {
			tooltip.style.display = "none";
			overlayCtx.clearRect(0, 0, overlayCanvas.width, overlayCanvas.height);
		});
		
		console.log("Multigraph rendered successfully");
	} catch (error) {
		console.error("Multigraph error:", error);
	}
})();
';

(new CWidgetView($data))
	->addItem($body)
	->addItem(new CScriptTag($graph_js))
	->show();
