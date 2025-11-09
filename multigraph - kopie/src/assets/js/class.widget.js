/**
 * Multigraph Widget JavaScript
 */
class CWidgetMultigraph extends CWidget {
    
    onInitialize() {
        this._canvas = null;
        this._overlay = null;
        this._graph_data = null;
        this._has_contents = false;
    }

    onActivate() {
        // Don't render on activate - wait for setContents
    }

    onResize() {
        if (this._graph_data) {
            this.renderGraph();
        }
    }

    onFeedback({type, value}) {
        // Handle dashboard time period changes
        if (type === CWidgetsData.DATA_TYPE_TIME_PERIOD && this.getFieldsReferredData().has('time_period')) {
            this._startUpdating();
            return true;
        }
        return false;
    }

    getUpdateRequestData() {
        return {
            ...super.getUpdateRequestData()
        };
    }

    setContents(response) {
        super.setContents(response);

        if (response.graph_data === undefined) {
            return;
        }

        this._graph_data = response.graph_data;
        this._has_contents = true;
        this.renderGraph();
    }

    _getValueAtTime(data, targetTime) {
        // Find value at specific timestamp (with linear interpolation)
        if (!data || data.length === 0) return 0;
        
        // If target is before first point, return first value
        if (targetTime <= data[0][0]) return data[0][1];
        
        // If target is after last point, return last value
        if (targetTime >= data[data.length - 1][0]) return data[data.length - 1][1];
        
        // Find surrounding points and interpolate
        for (let i = 0; i < data.length - 1; i++) {
            if (data[i][0] <= targetTime && targetTime <= data[i + 1][0]) {
                // Linear interpolation
                const t1 = data[i][0];
                const v1 = data[i][1];
                const t2 = data[i + 1][0];
                const v2 = data[i + 1][1];
                
                const ratio = (targetTime - t1) / (t2 - t1);
                return v1 + ratio * (v2 - v1);
            }
        }
        
        return data[data.length - 1][1];
    }

    renderGraph() {
        const container = this._target.querySelector('.multigraph-container');
        if (!container) {
            console.error('Multigraph container not found');
            return;
        }

        const graph_id = container.id;
        const canvas = document.getElementById(graph_id + '_canvas');
        const overlay = document.getElementById(graph_id + '_overlay');

        if (!canvas || !overlay) {
            console.error('Canvas elements not found');
            return;
        }

        if (!this._graph_data || !this._graph_data.series) {
            console.error('No graph data available');
            return;
        }

        this._canvas = canvas;
        this._overlay = overlay;

        const ctx = canvas.getContext('2d');
        const data = this._graph_data;

        // Set canvas size
        const rect = container.getBoundingClientRect();
        canvas.width = rect.width;
        canvas.height = rect.height;
        overlay.width = rect.width;
        overlay.height = rect.height;

        const width = canvas.width;
        const height = canvas.height;

        // Margins
        const marginTop = 40;
        const marginRight = 20;
        const marginBottom = 50;
        const marginLeft = 60;

        // Reserve space for legend
        let legendHeight = 0;
        let legendWidth = 0;
        const legend_position = parseInt(data.legend_position) || 0;
        
        if (legend_position === 2) { // bottom
            legendHeight = 30 + Math.ceil(data.series.length / 4) * 20;
        } else if (legend_position === 1) { // right
            legendWidth = 150;
        }

        const graphWidth = width - marginLeft - marginRight - legendWidth;
        const graphHeight = height - marginTop - marginBottom - legendHeight;

        // Find data range from actual data structure [timestamp, value]
        let minValue = Infinity;
        let maxValue = -Infinity;
        let minTime = Infinity;
        let maxTime = -Infinity;

        data.series.forEach(series => {
            if (!series.data || series.data.length === 0) return;
            
            series.data.forEach(point => {
                const timestamp = point[0];
                const value = point[1];
                
                if (timestamp < minTime) minTime = timestamp;
                if (timestamp > maxTime) maxTime = timestamp;
                if (value < minValue) minValue = value;
                if (value > maxValue) maxValue = value;
            });
        });

        // Handle y_min and y_max
        const y_min_str = (data.y_min || '').trim();
        const y_max_str = (data.y_max || '').trim();

        if (y_min_str !== '' && y_min_str.toLowerCase() !== 'auto') {
            const y_min_val = parseFloat(y_min_str);
            if (!isNaN(y_min_val)) {
                minValue = Math.min(minValue, y_min_val);
            }
        }

        if (y_max_str !== '' && y_max_str.toLowerCase() !== 'auto') {
            const y_max_val = parseFloat(y_max_str);
            if (!isNaN(y_max_val)) {
                maxValue = Math.max(maxValue, y_max_val);
            }
        }

        const valueRange = maxValue - minValue || 1;
        const timeRange = maxTime - minTime || 1;

        // Clear canvas
        ctx.clearRect(0, 0, width, height);

        // Draw grid
        if (data.show_grid) {
            ctx.strokeStyle = '#e0e0e0';
            ctx.lineWidth = 1;

            const grid_density_str = (data.grid_density || 'auto').trim().toLowerCase();
            let gridLines = 5;
            if (grid_density_str !== 'auto') {
                const parsed = parseInt(grid_density_str);
                if (!isNaN(parsed) && parsed > 0) {
                    gridLines = parsed;
                }
            }

            // Horizontal grid lines
            for (let i = 0; i <= gridLines; i++) {
                const y = marginTop + (graphHeight / gridLines) * i;
                ctx.beginPath();
                ctx.moveTo(marginLeft, y);
                ctx.lineTo(marginLeft + graphWidth, y);
                ctx.stroke();
            }

            // Vertical grid lines
            const timeGridLines = 6;
            for (let i = 0; i <= timeGridLines; i++) {
                const x = marginLeft + (graphWidth / timeGridLines) * i;
                ctx.beginPath();
                ctx.moveTo(x, marginTop);
                ctx.lineTo(x, marginTop + graphHeight);
                ctx.stroke();
            }
        }

        // Draw axes
        ctx.strokeStyle = '#333';
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.moveTo(marginLeft, marginTop);
        ctx.lineTo(marginLeft, marginTop + graphHeight);
        ctx.lineTo(marginLeft + graphWidth, marginTop + graphHeight);
        ctx.stroke();

        // Y-axis labels
        const yAxisColor = data.text_color_yaxis || '#000';
        ctx.fillStyle = yAxisColor;
        ctx.font = '12px Arial';
        ctx.textAlign = 'right';
        ctx.textBaseline = 'middle';

        const yLabelCount = 5;
        for (let i = 0; i <= yLabelCount; i++) {
            const value = maxValue - (valueRange / yLabelCount) * i;
            const y = marginTop + (graphHeight / yLabelCount) * i;
            ctx.fillText(value.toFixed(2), marginLeft - 10, y);
        }

        // Y-axis label
        if (data.y_axis_label) {
            ctx.save();
            ctx.translate(15, marginTop + graphHeight / 2);
            ctx.rotate(-Math.PI / 2);
            ctx.textAlign = 'center';
            ctx.fillStyle = yAxisColor;
            ctx.fillText(data.y_axis_label, 0, 0);
            ctx.restore();
        }

        // X-axis time labels with adaptive formatting
        const xAxisColor = data.text_color_xaxis || '#000';
        ctx.fillStyle = xAxisColor;
        ctx.textAlign = 'center';
        ctx.textBaseline = 'top';

        const xLabelCount = 6;
        const timeRangeSeconds = timeRange / 1000; // Convert ms to seconds
        
        // Determine format based on time range
        let dateFormat;
        if (timeRangeSeconds > 365 * 24 * 3600) {
            // More than 1 year: show year and month
            dateFormat = (date) => date.toLocaleDateString([], { year: 'numeric', month: 'short', day: 'numeric' });
        } else if (timeRangeSeconds > 30 * 24 * 3600) {
            // More than 30 days: show month and day
            dateFormat = (date) => date.toLocaleDateString([], { month: 'short', day: 'numeric' }) + ' ' + 
                                   date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
        } else if (timeRangeSeconds > 24 * 3600) {
            // More than 1 day: show day and time
            dateFormat = (date) => date.toLocaleDateString([], { month: 'short', day: 'numeric' }) + ' ' + 
                                   date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
        } else if (timeRangeSeconds > 3600) {
            // More than 1 hour: show HH:MM
            dateFormat = (date) => date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
        } else {
            // Less than 1 hour: show HH:MM:SS
            dateFormat = (date) => date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
        }
        
        for (let i = 0; i <= xLabelCount; i++) {
            const timestamp = minTime + (timeRange / xLabelCount) * i;
            const date = new Date(timestamp);
            const timeStr = dateFormat(date);
            const x = marginLeft + (graphWidth / xLabelCount) * i;
            ctx.fillText(timeStr, x, marginTop + graphHeight + 5);
        }

        // Draw series with fills (stacked - each series fills to next series below)
        const fill_opacity = parseFloat(data.fill_opacity) || 0;

        // Sort series by their maximum value to determine stacking order (highest on top)
        const sortedSeries = [...data.series].sort((a, b) => {
            const maxA = Math.max(...a.data.map(p => p[1]));
            const maxB = Math.max(...b.data.map(p => p[1]));
            return maxB - maxA; // Descending order
        });

        // Draw fills from bottom to top
        for (let seriesIdx = sortedSeries.length - 1; seriesIdx >= 0; seriesIdx--) {
            const series = sortedSeries[seriesIdx];
            if (!series.data || series.data.length === 0) continue;

            if (fill_opacity > 0) {
                // Convert hex color to rgba with opacity
                const hexColor = series.color;
                const r = parseInt(hexColor.slice(1, 3), 16);
                const g = parseInt(hexColor.slice(3, 5), 16);
                const b = parseInt(hexColor.slice(5, 7), 16);
                
                ctx.fillStyle = `rgba(${r}, ${g}, ${b}, ${fill_opacity})`;
                ctx.beginPath();

                // Find the next series below (for stacking) or use baseline
                const nextSeriesBelow = seriesIdx < sortedSeries.length - 1 ? sortedSeries[seriesIdx + 1] : null;

                // Start from left edge
                const firstTimestamp = series.data[0][0];
                const firstValue = series.data[0][1];
                const firstX = marginLeft + ((firstTimestamp - minTime) / timeRange) * graphWidth;
                const firstY = marginTop + graphHeight - ((firstValue - minValue) / valueRange) * graphHeight;

                // Start from bottom line or baseline
                if (nextSeriesBelow && nextSeriesBelow.data.length > 0) {
                    // Start from the point on the series below
                    const belowValue = this._getValueAtTime(nextSeriesBelow.data, firstTimestamp);
                    const belowY = marginTop + graphHeight - ((belowValue - minValue) / valueRange) * graphHeight;
                    ctx.moveTo(firstX, belowY);
                } else {
                    // Start from baseline (X-axis)
                    ctx.moveTo(firstX, marginTop + graphHeight);
                }

                // Draw top line of current series
                series.data.forEach(point => {
                    const timestamp = point[0];
                    const value = point[1];
                    const x = marginLeft + ((timestamp - minTime) / timeRange) * graphWidth;
                    const y = marginTop + graphHeight - ((value - minValue) / valueRange) * graphHeight;
                    ctx.lineTo(x, y);
                });

                // Draw back along bottom line (series below or baseline)
                if (nextSeriesBelow && nextSeriesBelow.data.length > 0) {
                    // Go backwards along the series below
                    for (let i = series.data.length - 1; i >= 0; i--) {
                        const timestamp = series.data[i][0];
                        const belowValue = this._getValueAtTime(nextSeriesBelow.data, timestamp);
                        const x = marginLeft + ((timestamp - minTime) / timeRange) * graphWidth;
                        const y = marginTop + graphHeight - ((belowValue - minValue) / valueRange) * graphHeight;
                        ctx.lineTo(x, y);
                    }
                } else {
                    // Close to baseline
                    const lastTimestamp = series.data[series.data.length - 1][0];
                    const lastX = marginLeft + ((lastTimestamp - minTime) / timeRange) * graphWidth;
                    ctx.lineTo(lastX, marginTop + graphHeight);
                }

                ctx.closePath();
                ctx.fill();
            }
        }

        // Draw lines on top of fills (in original order)
        data.series.forEach(series => {
            if (!series.data || series.data.length === 0) return;

            ctx.strokeStyle = series.color;
            ctx.lineWidth = 2;
            ctx.beginPath();

            series.data.forEach((point, idx) => {
                const timestamp = point[0];
                const value = point[1];
                const x = marginLeft + ((timestamp - minTime) / timeRange) * graphWidth;
                const y = marginTop + graphHeight - ((value - minValue) / valueRange) * graphHeight;

                if (idx === 0) {
                    ctx.moveTo(x, y);
                } else {
                    ctx.lineTo(x, y);
                }
            });

            ctx.stroke();
        });

        // Draw legend
        const legendColor = data.text_color_legend || '#000';
        ctx.font = '12px Arial';
        ctx.textBaseline = 'middle';

        switch (legend_position) {
            case 0: // top-left
                let legendY = 10;
                data.series.forEach(series => {
                    ctx.fillStyle = series.color;
                    ctx.fillRect(10, legendY, 15, 15);
                    ctx.fillStyle = legendColor;
                    ctx.textAlign = 'left';
                    ctx.fillText(series.name, 30, legendY + 7);
                    legendY += 20;
                });
                break;

            case 1: // right
                let rightLegendY = marginTop;
                const rightLegendX = marginLeft + graphWidth + 10;
                data.series.forEach(series => {
                    ctx.fillStyle = series.color;
                    ctx.fillRect(rightLegendX, rightLegendY, 15, 15);
                    ctx.fillStyle = legendColor;
                    ctx.textAlign = 'left';
                    ctx.fillText(series.name, rightLegendX + 20, rightLegendY + 7);
                    rightLegendY += 20;
                });
                break;

            case 2: // bottom
                const bottomLegendY = height - legendHeight + 10;
                let bottomLegendX = 10;
                const itemsPerRow = 4;
                data.series.forEach((series, idx) => {
                    if (idx > 0 && idx % itemsPerRow === 0) {
                        bottomLegendX = 10;
                    }
                    ctx.fillStyle = series.color;
                    ctx.fillRect(bottomLegendX, bottomLegendY + Math.floor(idx / itemsPerRow) * 20, 15, 15);
                    ctx.fillStyle = legendColor;
                    ctx.textAlign = 'left';
                    ctx.fillText(series.name, bottomLegendX + 20, bottomLegendY + Math.floor(idx / itemsPerRow) * 20 + 7);
                    bottomLegendX += width / itemsPerRow;
                });
                break;
        }

        // Hover tooltip functionality
        let tooltipDiv = container.querySelector('.multigraph-tooltip');
        if (!tooltipDiv) {
            tooltipDiv = document.createElement('div');
            tooltipDiv.className = 'multigraph-tooltip';
            tooltipDiv.style.cssText = 'position:absolute;background:rgba(0,0,0,0.8);color:#fff;padding:8px;border-radius:4px;font-size:12px;pointer-events:none;display:none;z-index:1000;white-space:nowrap;';
            container.appendChild(tooltipDiv);
        }

        // Set canvas size for overlay to match main canvas
        overlay.width = canvas.width;
        overlay.height = canvas.height;
        overlay.style.position = 'absolute';
        overlay.style.top = '0';
        overlay.style.left = '0';
        overlay.style.pointerEvents = 'auto'; // CRITICAL: Enable pointer events on overlay
        const overlayCtx = overlay.getContext('2d');

        console.log('Multigraph: Setting up hover on overlay', overlay.id, 'size:', overlay.width, 'x', overlay.height);

        // Store references for event handlers
        const graphBounds = { marginLeft, marginTop, graphWidth, graphHeight };
        const timeBounds = { minTime, maxTime, timeRange };
        const valueBounds = { minValue, maxValue, valueRange };

        // Remove old mousemove listener if exists
        if (this._mousemoveHandler) {
            overlay.removeEventListener('mousemove', this._mousemoveHandler);
            overlay.removeEventListener('mouseleave', this._mouseleaveHandler);
        }

        // Create new handlers
        this._mousemoveHandler = (e) => {
            const rect = overlay.getBoundingClientRect();
            const mouseX = e.clientX - rect.left;
            const mouseY = e.clientY - rect.top;

            console.log('Multigraph: Mouse move at', mouseX, mouseY);

            // Clear overlay
            overlayCtx.clearRect(0, 0, overlay.width, overlay.height);

            // Check if mouse is in graph area
            if (mouseX < graphBounds.marginLeft || mouseX > graphBounds.marginLeft + graphBounds.graphWidth || 
                mouseY < graphBounds.marginTop || mouseY > graphBounds.marginTop + graphBounds.graphHeight) {
                tooltipDiv.style.display = 'none';
                return;
            }

            // Find nearest points based on time
            const relativeX = (mouseX - graphBounds.marginLeft) / graphBounds.graphWidth;
            const targetTime = timeBounds.minTime + relativeX * timeBounds.timeRange;
            let tooltipHTML = '';
            let hasData = false;

            data.series.forEach(series => {
                if (!series.data || series.data.length === 0) return;

                // Find closest point by timestamp
                let closestPoint = series.data[0];
                let minDist = Math.abs(series.data[0][0] - targetTime);

                series.data.forEach(point => {
                    const dist = Math.abs(point[0] - targetTime);
                    if (dist < minDist) {
                        minDist = dist;
                        closestPoint = point;
                    }
                });

                const timestamp = closestPoint[0];
                const value = closestPoint[1];

                // Draw highlight circle
                const pointX = graphBounds.marginLeft + ((timestamp - timeBounds.minTime) / timeBounds.timeRange) * graphBounds.graphWidth;
                const pointY = graphBounds.marginTop + graphBounds.graphHeight - ((value - valueBounds.minValue) / valueBounds.valueRange) * graphBounds.graphHeight;

                overlayCtx.fillStyle = series.color;
                overlayCtx.beginPath();
                overlayCtx.arc(pointX, pointY, 5, 0, Math.PI * 2);
                overlayCtx.fill();

                // Add to tooltip
                const time = new Date(timestamp).toLocaleTimeString();
                tooltipHTML += `<div><span style="color:${series.color}">●</span> ${series.name}: ${value.toFixed(2)} (${time})</div>`;
                hasData = true;
            });

            if (hasData) {
                tooltipDiv.innerHTML = tooltipHTML;
                tooltipDiv.style.display = 'block';
                
                // Position tooltip smartly to avoid going off screen
                let tooltipX = mouseX + 15;
                let tooltipY = mouseY - 10;
                
                // Check if tooltip would go off right edge
                const tooltipRect = tooltipDiv.getBoundingClientRect();
                if (tooltipX + tooltipRect.width > width) {
                    tooltipX = mouseX - tooltipRect.width - 15;
                }
                
                tooltipDiv.style.left = tooltipX + 'px';
                tooltipDiv.style.top = tooltipY + 'px';
            } else {
                tooltipDiv.style.display = 'none';
            }
        };

        this._mouseleaveHandler = () => {
            overlayCtx.clearRect(0, 0, overlay.width, overlay.height);
            tooltipDiv.style.display = 'none';
        };

        // Attach event listeners
        overlay.addEventListener('mousemove', this._mousemoveHandler);
        overlay.addEventListener('mouseleave', this._mouseleaveHandler);

        console.log('Multigraph: Event listeners attached to overlay', overlay.id);
        console.log('Multigraph: Overlay style:', overlay.style.cssText);
        console.log('Multigraph rendered successfully');
    }

    // Helper function to get interpolated value at a specific timestamp
    _getValueAtTime(dataPoints, targetTime) {
        if (!dataPoints || dataPoints.length === 0) return 0;

        // Find surrounding points
        let before = dataPoints[0];
        let after = dataPoints[dataPoints.length - 1];

        for (let i = 0; i < dataPoints.length - 1; i++) {
            if (dataPoints[i][0] <= targetTime && dataPoints[i + 1][0] >= targetTime) {
                before = dataPoints[i];
                after = dataPoints[i + 1];
                break;
            }
        }

        // If exact match
        if (before[0] === targetTime) return before[1];
        if (after[0] === targetTime) return after[1];

        // Linear interpolation
        const timeDiff = after[0] - before[0];
        if (timeDiff === 0) return before[1];

        const ratio = (targetTime - before[0]) / timeDiff;
        return before[1] + ratio * (after[1] - before[1]);
    }
}

