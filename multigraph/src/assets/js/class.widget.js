/**
 * ============================================================================
 * File: class.widget.js
 * Created: 2025-11-09 19:30:00
 * 
 * Lead & Architecture: lukas@plachy.eu
 * Development: Claude Sonnet 4 (AI Assistant, Anthropic)
 * 
 * License: MIT (unless specified otherwise in project root)
 * ============================================================================
 * 
 * PURPOSE:
 * Main widget JavaScript class - handles client-side rendering and interactivity.
 * Extends Zabbix CWidget to integrate with dashboard framework.
 * Implements HTML5 Canvas-based graph rendering with interactive features.
 * 
 * RELATIONS:
 * - Extends: CWidget (Zabbix dashboard widget framework)
 * - Receives data from: views/widget.view.php via setVar('graph_data', ...)
 * - Uses data from: includes/GraphData.php (prepared server-side)
 * - Styled by: assets/css/multigraph.css
 * 
 * FEATURES:
 * - Client-side graph rendering using HTML5 Canvas
 * - Responsive to widget resize
 * - Interactive hover with value display
 * - Dashboard timeframe integration
 * - Adaptive X-axis time formatting
 * - Stacked area fills
 * - Configurable colors, legend, grid
 * 
 * LIFECYCLE:
 * 1. onInitialize() - Set up instance variables
 * 2. setContents(response) - Receive data from server
 * 3. renderGraph() - Draw graph on canvas
 * 4. onResize() - Redraw on widget size change
 * 5. onFeedback() - Handle dashboard updates (timeframe changes)
 */

/**
 * Multigraph Widget Class
 * 
 * Client-side widget implementation extending Zabbix's CWidget base class.
 * Handles all graph rendering and user interactions.
 * 
 * @extends CWidget
 */
class CWidgetMultigraph extends CWidget {
    
    /**
     * Initialize widget instance variables
     * 
     * PURPOSE:
     * Sets up instance state before any data is received.
     * Called once when widget is first created.
     * 
     * DEPENDENCIES:
     * - Called by: Zabbix dashboard framework (CWidget lifecycle)
     * 
     * @returns {void}
     */
    onInitialize() {
        this._canvas = null;        // Main canvas element for graph drawing
        this._overlay = null;       // Overlay canvas for hover effects
        this._graph_data = null;    // Graph data from server (series, config)
        this._has_contents = false; // Flag indicating if data has been loaded
    }

    /**
     * Widget activation handler
     * 
     * PURPOSE:
     * Called when widget becomes visible in dashboard.
     * Intentionally empty - we wait for setContents() to have data before rendering.
     * 
     * DEPENDENCIES:
     * - Called by: Zabbix dashboard framework (CWidget lifecycle)
     * 
     * @returns {void}
     */
    onActivate() {
        // Don't render on activate - wait for setContents
    }

    /**
     * Widget resize handler
     * 
     * PURPOSE:
     * Redraws graph when widget is resized by user.
     * Ensures graph scales properly to new dimensions.
     * 
     * DEPENDENCIES:
     * - Called by: Zabbix dashboard framework when widget size changes
     * - Calls: renderGraph() if data is available
     * 
     * @returns {void}
     */
    onResize() {
        if (this._graph_data) {
            this.renderGraph(); // Redraw graph with new canvas dimensions
        }
    }

    /**
     * Dashboard feedback handler
     * 
     * PURPOSE:
     * Handles updates from dashboard (e.g., timeframe selector changes).
     * Triggers widget data refresh when time period changes.
     * 
     * DEPENDENCIES:
     * - Called by: Zabbix dashboard framework when dashboard state changes
     * - Triggers: Widget data refresh via _startUpdating()
     * 
     * @param {Object} feedback Feedback object from dashboard
     * @param {string} feedback.type Type of feedback (e.g., DATA_TYPE_TIME_PERIOD)
     * @param {*} feedback.value New value for the feedback type
     * 
     * @returns {boolean} True if feedback was handled, false otherwise
     */
    onFeedback({type, value}) {
        // Handle dashboard time period changes
        if (type === CWidgetsData.DATA_TYPE_TIME_PERIOD && this.getFieldsReferredData().has('time_period')) {
            this._startUpdating(); // Trigger widget refresh with new timeframe
            return true;
        }
        
        // Handle dashboard host changes (template dashboards on host pages)
        if (type === CWidgetsData.DATA_TYPE_HOST_ID && this.getFieldsReferredData().has('hostids')) {
            this._startUpdating(); // Trigger widget refresh with new host
            return true;
        }
        
        return false;
    }

    /**
     * Get request data for widget update
     * 
     * PURPOSE:
     * Prepares data to send when requesting widget refresh from server.
     * Merges widget-specific data with base widget data.
     * 
     * DEPENDENCIES:
     * - Called by: Zabbix framework before making update request
     * - Calls: super.getUpdateRequestData() for base data
     * 
     * @returns {Object} Request data object to send to server
     */
    getUpdateRequestData() {
        return {
            ...super.getUpdateRequestData()
        };
    }

    /**
     * Set widget contents from server response
     * 
     * PURPOSE:
     * Receives graph data from server and triggers initial render.
     * Main entry point for displaying graph after data is fetched.
     * 
     * DEPENDENCIES:
     * - Called by: Zabbix framework after receiving server response
     * - Receives: Data prepared by actions/WidgetView.php
     * - Calls: renderGraph() to display the data
     * 
     * DATA STRUCTURE:
     * response.graph_data = {
     *   series: [{name, data: [[ts_ms, val], ...], color, units}],
     *   legend_position: int,
     *   fill_opacity: float,
     *   y_axis_label: string,
     *   y_min: string|number,
     *   y_max: string|number,
     *   show_grid: bool,
     *   grid_density: string|number,
     *   text_color_legend: string,
     *   text_color_xaxis: string,
     *   text_color_yaxis: string
     * }
     * 
     * @param {Object} response Server response object
     * @param {Object} response.graph_data Graph data and configuration
     * 
     * @returns {void}
     */
    setContents(response) {
        super.setContents(response);

        if (response.graph_data === undefined) {
            return;
        }

        this._graph_data = response.graph_data; // Store graph data for rendering
        this._has_contents = true;               // Mark as having valid data
        this.renderGraph();                      // Trigger initial render
    }

    /**
     * Get interpolated value at specific time
     * 
     * PURPOSE:
     * Finds or interpolates data value at a specific timestamp.
     * Used for hover tooltips to show precise values.
     * 
     * ALGORITHM:
     * 1. Find data points before and after target time
     * 2. If exact match exists, return it
     * 3. Otherwise, linearly interpolate between surrounding points
     * 
     * @param {Array<Array<number>>} data Time series data [[timestamp_ms, value], ...]
     * @param {number} targetTime Target timestamp in milliseconds
     * 
     * @returns {number|null} Interpolated value or null if no data
     * 
     * @private
     */
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

    /**
     * Main graph rendering function
     * 
     * PURPOSE:
     * Renders complete graph using HTML5 Canvas with all series, axes, legend, grid.
     * Handles responsive sizing, adaptive time formatting, stacked area fills.
     * Sets up interactive hover tooltips with interpolated values.
     * 
     * ALGORITHM:
     * 1. Find DOM elements (container, canvas, overlay)
     * 2. Calculate layout dimensions (margins, legend space)
     * 3. Find data ranges (min/max time and value)
     * 4. Apply user-specified y_min/y_max overrides
     * 5. Draw grid (if enabled)
     * 6. Draw axes and axis labels
     * 7. Draw stacked area fills (bottom-to-top)
     * 8. Draw line series (top layer)
     * 9. Draw legend (position-dependent layout)
     * 10. Set up hover event handlers with tooltip
     * 
     * DEPENDENCIES:
     * - Called by: setContents() on initial render, onResize() on widget resize
     * - Uses: this._graph_data (set by setContents from server response)
     * - Calls: _getValueAtTime() for hover interpolation
     * 
     * DATA STRUCTURE USED:
     * this._graph_data = {
     *   series: [{name, data: [[ts_ms, val], ...], color, units}],
     *   legend_position: 0=top-left, 1=right, 2=bottom,
     *   fill_opacity: 0-1,
     *   y_axis_label: string,
     *   y_min: 'auto' | number,
     *   y_max: 'auto' | number,
     *   show_grid: boolean,
     *   grid_density: 'auto' | number,
     *   text_color_legend: hex color,
     *   text_color_xaxis: hex color,
     *   text_color_yaxis: hex color
     * }
     * 
     * RENDERING STRATEGY:
     * - Canvas layers: Main canvas (graph) + overlay canvas (hover effects)
     * - Coordinate system: Canvas pixels with margins for axes/legend
     * - Time formatting: Adaptive based on time range (year/month/day/hour/minute/second)
     * - Stacking: Series sorted by max value, fills drawn bottom-to-top
     * - Hover: Overlay canvas with event listeners, interpolates values between points
     * 
     * @returns {void}
     */
    renderGraph() {
        // === PHASE 1: DOM Setup and Validation ===
        const container = this._target.querySelector('.multigraph-container');
        if (!container) {
            console.error('Multigraph container not found');
            return;
        }

        const graph_id = container.id;
        const canvas = document.getElementById(graph_id + '_canvas');
        let overlay = document.getElementById(graph_id + '_overlay');

        if (!canvas) {
            console.error('Canvas element not found');
            return;
        }

        // RECREATE overlay canvas from scratch to fix rendering bug
        if (overlay) {
            overlay.remove();
        }
        overlay = document.createElement('canvas');
        overlay.id = graph_id + '_overlay';
        container.appendChild(overlay);

        if (!this._graph_data || !this._graph_data.series) {
            console.error('No graph data available');
            return;
        }

        this._canvas = canvas;
        this._overlay = overlay;

        const data = this._graph_data;       // Alias for readability

        // === PHASE 2: Layout Calculation ===
        // Set canvas size to match container
        const rect = container.getBoundingClientRect();
        const dpr = window.devicePixelRatio || 1;
        
        // Set canvas dimensions (both internal and CSS)
        canvas.width = rect.width * dpr;
        canvas.height = rect.height * dpr;
        canvas.style.cssText = `position: absolute; top: 0; left: 0; width: ${rect.width}px; height: ${rect.height}px; z-index: 1;`;
        
        const ctx = canvas.getContext('2d');
        ctx.scale(dpr, dpr);
        
        // Set overlay dimensions (both internal and CSS)
        // CRITICAL: Canvas MUST have explicit pixel dimensions AND matching canvas.width/height
        overlay.width = rect.width * dpr;
        overlay.height = rect.height * dpr;
        // Clear any existing styles first
        overlay.style.cssText = '';
        // Set styles individually with specific values
        overlay.style.position = 'absolute';
        overlay.style.top = '0';
        overlay.style.left = '0';
        overlay.style.width = rect.width + 'px';
        overlay.style.height = rect.height + 'px';
        overlay.style.pointerEvents = 'none';
        overlay.style.zIndex = '100';
        overlay.style.display = 'block';
        overlay.style.visibility = 'visible';
        overlay.style.opacity = '1';
        // Force hardware acceleration
        overlay.style.transform = 'translate3d(0,0,0)';
        overlay.style.willChange = 'transform';
        
        const overlayCtx = overlay.getContext('2d', { alpha: true });
        // USE SAME APPROACH AS MAIN CANVAS - scale and draw in CSS coordinates
        overlayCtx.scale(dpr, dpr);
        
        // Force browser reflow to ensure dimensions are applied
        void overlay.offsetHeight;

        const width = canvas.width;
        const height = canvas.height;

        // Margins for axes and labels
        const marginTop = 40;    // Space for top-left legend or padding
        const marginRight = 20;  // Padding
        const marginBottom = 50; // Space for X-axis labels and bottom legend
        const marginLeft = 60;   // Space for Y-axis labels and Y-axis label

        // Reserve space for legend based on position
        let legendHeight = 0; // Additional space if legend at bottom
        let legendWidth = 0;  // Additional space if legend at right
        const legend_position = parseInt(data.legend_position) || 0;
        
        if (legend_position === 2) { // Bottom: 4 items per row
            legendHeight = 30 + Math.ceil(data.series.length / 4) * 20;
        } else if (legend_position === 1) { // Right: vertical list
            legendWidth = 150;
        }
        // Position 0 (top-left) overlays graph, no space reservation needed

        // Calculate actual graph area
        const graphWidth = width - marginLeft - marginRight - legendWidth;
        const graphHeight = height - marginTop - marginBottom - legendHeight;

        // === PHASE 3: Data Range Discovery ===
        // Find min/max time and value from all series
        let minValue = Infinity;
        let maxValue = -Infinity;
        let minTime = Infinity;
        let maxTime = -Infinity;

        data.series.forEach(series => {
            if (!series.data || series.data.length === 0) return;
            
            series.data.forEach(point => {
                const timestamp = point[0]; // Milliseconds since epoch
                const value = point[1];     // Numeric value
                
                if (timestamp < minTime) minTime = timestamp;
                if (timestamp > maxTime) maxTime = timestamp;
                if (value < minValue) minValue = value;
                if (value > maxValue) maxValue = value;
            });
        });

        // === PHASE 4: Apply Y-Axis Overrides ===
        // User can specify fixed Y-axis min/max or use 'auto'
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

        const valueRange = maxValue - minValue || 1; // Avoid division by zero
        const timeRange = maxTime - minTime || 1;    // Avoid division by zero

        // === PHASE 4.5: Preprocess Data for Missing Data Handling ===
        const missing_data = data.missing_data !== undefined ? data.missing_data : 1; // 0=none(gap), 1=connected, 2=zero
        
        // Augment each series with gap detection metadata
        data.series.forEach(series => {
            if (!series.data || series.data.length < 2) return;
            
            // Calculate average interval for this series
            let avgInterval = 0;
            for (let i = 1; i < Math.min(series.data.length, 10); i++) {
                avgInterval += series.data[i][0] - series.data[i-1][0];
            }
            avgInterval /= Math.min(series.data.length - 1, 9);
            const gapThreshold = avgInterval * 2;
            
            // Create processed data with gap information
            series.processedData = [];
            
            for (let i = 0; i < series.data.length; i++) {
                const point = series.data[i];
                const timestamp = point[0];
                const value = point[1];
                
                let hasGapBefore = false;
                if (i > 0) {
                    const prevTimestamp = series.data[i - 1][0];
                    const actualInterval = timestamp - prevTimestamp;
                    hasGapBefore = actualInterval > gapThreshold;
                }
                
                // Create processed point with metadata
                const processedPoint = {
                    timestamp: timestamp,
                    value: value,
                    hasGapBefore: hasGapBefore,
                    drawValue: value,  // Will be modified for mode=2 (treat as 0)
                    draw: true         // Will be false for segments to skip
                };
                
                // Handle missing data modes
                if (hasGapBefore) {
                    if (missing_data === 2) {
                        // Treat as 0: Insert virtual zero points at gap boundaries
                        // Add zero at end of previous segment
                        series.processedData.push({
                            timestamp: series.data[i - 1][0],
                            value: minValue, // X-axis level
                            hasGapBefore: false,
                            drawValue: minValue,
                            draw: true,
                            isVirtualZero: true
                        });
                        // Add zero at start of current segment
                        series.processedData.push({
                            timestamp: timestamp,
                            value: minValue, // X-axis level
                            hasGapBefore: false,
                            drawValue: minValue,
                            draw: true,
                            isVirtualZero: true
                        });
                    }
                }
                
                series.processedData.push(processedPoint);
            }
        });

        // === PHASE 5: Clear Canvas ===
        ctx.clearRect(0, 0, width, height);

        // === PHASE 6: Draw Grid (Optional) ===
        if (data.show_grid) {
            ctx.strokeStyle = '#e0e0e0'; // Light gray grid lines
            ctx.lineWidth = 1;

            // Parse grid density (number of lines or 'auto')
            const grid_density_str = (data.grid_density || 'auto').trim().toLowerCase();
            let gridLines = 5; // Default
            if (grid_density_str !== 'auto') {
                const parsed = parseInt(grid_density_str);
                if (!isNaN(parsed) && parsed > 0) {
                    gridLines = parsed;
                }
            }

            // Horizontal grid lines (Y-axis divisions)
            for (let i = 0; i <= gridLines; i++) {
                const y = marginTop + (graphHeight / gridLines) * i;
                ctx.beginPath();
                ctx.moveTo(marginLeft, y);
                ctx.lineTo(marginLeft + graphWidth, y);
                ctx.stroke();
            }

            // Vertical grid lines (X-axis divisions)
            const timeGridLines = 6; // Fixed for readability
            for (let i = 0; i <= timeGridLines; i++) {
                const x = marginLeft + (graphWidth / timeGridLines) * i;
                ctx.beginPath();
                ctx.moveTo(x, marginTop);
                ctx.lineTo(x, marginTop + graphHeight);
                ctx.stroke();
            }
        }

        // === PHASE 7: Draw Axes ===
        ctx.strokeStyle = '#333'; // Dark gray axes
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.moveTo(marginLeft, marginTop);                       // Y-axis top
        ctx.lineTo(marginLeft, marginTop + graphHeight);         // Y-axis bottom
        ctx.lineTo(marginLeft + graphWidth, marginTop + graphHeight); // X-axis right
        ctx.stroke();

        // === Determine graph type early for conditional rendering ===
        const graph_type = data.graph_type !== undefined ? data.graph_type : 0; // 0=line, 1=bar, 2=distribution
        console.log('DEBUG: graph_type =', graph_type, 'typeof=', typeof graph_type);

        // Define axis colors early (used in multiple phases)
        const yAxisColor = data.text_color_yaxis || '#000';
        const xAxisColor = data.text_color_xaxis || '#000';

        // === PHASE 8: Y-Axis Labels ===
        // Skip for distribution mode (draws its own count-based Y-axis)
        
        console.log('DEBUG: Before Y-axis rendering');
        
        if (graph_type !== 2) {
            ctx.fillStyle = yAxisColor;
            ctx.font = '12px Arial';
            ctx.textAlign = 'right';
            ctx.textBaseline = 'middle';

            const yLabelCount = 5; // Number of Y-axis labels
            for (let i = 0; i <= yLabelCount; i++) {
                const value = maxValue - (valueRange / yLabelCount) * i; // Top to bottom
                const y = marginTop + (graphHeight / yLabelCount) * i;
                ctx.fillText(value.toFixed(2), marginLeft - 10, y);
            }

            // Y-axis title (rotated)
            if (data.y_axis_label) {
                ctx.save();
                ctx.translate(15, marginTop + graphHeight / 2); // Center on Y-axis
                ctx.rotate(-Math.PI / 2);                       // Rotate 90° counter-clockwise
                ctx.textAlign = 'center';
                ctx.fillStyle = yAxisColor;
                ctx.fillText(data.y_axis_label, 0, 0);
                ctx.restore();
            }
        }

        // === PHASE 9: X-Axis Time Labels (Adaptive Formatting) ===
        // Skip for distribution mode (draws its own bin-range X-axis)
        console.log('DEBUG: Before X-axis rendering');
        if (graph_type !== 2) {
            ctx.fillStyle = xAxisColor;
            ctx.textAlign = 'center';
            ctx.textBaseline = 'top';

            const xLabelCount = 6; // Number of X-axis labels
            const timeRangeSeconds = timeRange / 1000; // Convert milliseconds to seconds
            
            // Choose date format based on time range
            let dateFormat;
            if (timeRangeSeconds > 365 * 24 * 3600) {
                // More than 1 year: show "Nov 2025"
                dateFormat = (date) => date.toLocaleDateString([], { year: 'numeric', month: 'short', day: 'numeric' });
            } else if (timeRangeSeconds > 30 * 24 * 3600) {
                // More than 30 days: show "Nov 5 14:30"
                dateFormat = (date) => date.toLocaleDateString([], { month: 'short', day: 'numeric' }) + ' ' + 
                                       date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
            } else if (timeRangeSeconds > 24 * 3600) {
                // More than 1 day: show "Nov 5 14:30"
                dateFormat = (date) => date.toLocaleDateString([], { month: 'short', day: 'numeric' }) + ' ' + 
                                       date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
            } else if (timeRangeSeconds > 3600) {
                // More than 1 hour: show "14:30"
                dateFormat = (date) => date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
            } else {
                // Less than 1 hour: show "14:30:45"
                dateFormat = (date) => date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
            }
            
            for (let i = 0; i <= xLabelCount; i++) {
                const timestamp = minTime + (timeRange / xLabelCount) * i;
                const date = new Date(timestamp);
                const timeStr = dateFormat(date);
                const x = marginLeft + (graphWidth / xLabelCount) * i;
                ctx.fillText(timeStr, x, marginTop + graphHeight + 5);
            }
        }

        // === PHASE 10: Draw Stacked Area Fills ===
        const fill_opacity = parseFloat(data.fill_opacity) || 0;

        // Skip fill rendering for bar charts and distribution graphs - fills only for line graphs
        if (graph_type === 0 && fill_opacity > 0) {
            // Sort series by maximum value (descending) for stacking order
            // Highest values on top, lowest at bottom
            // Avoid spread operator for large arrays
            const sortedSeries = data.series.slice().sort((a, b) => {
                let maxA = 0;
                if (a.processedData && a.processedData.length > 0) {
                    maxA = a.processedData[0].drawValue;
                    for (let i = 1; i < a.processedData.length; i++) {
                        if (a.processedData[i].drawValue > maxA) maxA = a.processedData[i].drawValue;
                    }
                }
                let maxB = 0;
                if (b.processedData && b.processedData.length > 0) {
                    maxB = b.processedData[0].drawValue;
                    for (let i = 1; i < b.processedData.length; i++) {
                        if (b.processedData[i].drawValue > maxB) maxB = b.processedData[i].drawValue;
                    }
                }
                return maxB - maxA; // Descending order
            });

            // Draw fills from bottom to top (reverse iteration)
            // Each series fills the area between itself and the series below
            for (let seriesIdx = sortedSeries.length - 1; seriesIdx >= 0; seriesIdx--) {
                const series = sortedSeries[seriesIdx];
                if (!series.processedData || series.processedData.length === 0) continue;

                // Convert hex color to RGBA with user-specified opacity
                const hexColor = series.color;
                const r = parseInt(hexColor.slice(1, 3), 16);
                const g = parseInt(hexColor.slice(3, 5), 16);
                const b = parseInt(hexColor.slice(5, 7), 16);
                
                ctx.fillStyle = `rgba(${r}, ${g}, ${b}, ${fill_opacity})`;

                // Find the next series below for stacking, or use X-axis baseline
                const nextSeriesBelow = seriesIdx < sortedSeries.length - 1 ? sortedSeries[seriesIdx + 1] : null;

                if (missing_data === 0) {
                    // None mode: Draw separate fill segments at gaps
                    let segmentStart = 0;
                    for (let i = 1; i <= series.processedData.length; i++) {
                        const isGap = i < series.processedData.length && series.processedData[i].hasGapBefore;
                        const isEnd = i === series.processedData.length;
                        
                        if (isGap || isEnd) {
                            // Draw segment from segmentStart to i-1
                            const segment = series.processedData.slice(segmentStart, i);
                            if (segment.length > 0) {
                                ctx.beginPath();
                                
                                const firstX = marginLeft + ((segment[0].timestamp - minTime) / timeRange) * graphWidth;
                                if (nextSeriesBelow && nextSeriesBelow.processedData && nextSeriesBelow.processedData.length > 0) {
                                    const belowValue = this._getValueAtTime(nextSeriesBelow.data, segment[0].timestamp);
                                    const belowY = marginTop + graphHeight - ((belowValue - minValue) / valueRange) * graphHeight;
                                    ctx.moveTo(firstX, belowY);
                                } else {
                                    ctx.moveTo(firstX, marginTop + graphHeight);
                                }
                                
                                segment.forEach(point => {
                                    const x = marginLeft + ((point.timestamp - minTime) / timeRange) * graphWidth;
                                    const y = marginTop + graphHeight - ((point.drawValue - minValue) / valueRange) * graphHeight;
                                    ctx.lineTo(x, y);
                                });
                                
                                if (nextSeriesBelow && nextSeriesBelow.processedData && nextSeriesBelow.processedData.length > 0) {
                                    for (let j = segment.length - 1; j >= 0; j--) {
                                        const belowValue = this._getValueAtTime(nextSeriesBelow.data, segment[j].timestamp);
                                        const x = marginLeft + ((segment[j].timestamp - minTime) / timeRange) * graphWidth;
                                        const y = marginTop + graphHeight - ((belowValue - minValue) / valueRange) * graphHeight;
                                        ctx.lineTo(x, y);
                                    }
                                } else {
                                    const lastX = marginLeft + ((segment[segment.length-1].timestamp - minTime) / timeRange) * graphWidth;
                                    ctx.lineTo(lastX, marginTop + graphHeight);
                                }
                                
                                ctx.closePath();
                                ctx.fill();
                            }
                            segmentStart = i;
                        }
                    }
                } else {
                    // Connected or Treat as 0: Draw continuous fill using processed data
                    ctx.beginPath();
                    
                    const firstTimestamp = series.processedData[0].timestamp;
                    const firstX = marginLeft + ((firstTimestamp - minTime) / timeRange) * graphWidth;

                    if (nextSeriesBelow && nextSeriesBelow.processedData && nextSeriesBelow.processedData.length > 0) {
                        const belowValue = this._getValueAtTime(nextSeriesBelow.data, firstTimestamp);
                        const belowY = marginTop + graphHeight - ((belowValue - minValue) / valueRange) * graphHeight;
                        ctx.moveTo(firstX, belowY);
                    } else {
                        ctx.moveTo(firstX, marginTop + graphHeight);
                    }

                    series.processedData.forEach(point => {
                        const x = marginLeft + ((point.timestamp - minTime) / timeRange) * graphWidth;
                        const y = marginTop + graphHeight - ((point.drawValue - minValue) / valueRange) * graphHeight;
                        ctx.lineTo(x, y);
                    });

                    if (nextSeriesBelow && nextSeriesBelow.processedData && nextSeriesBelow.processedData.length > 0) {
                        for (let i = series.processedData.length - 1; i >= 0; i--) {
                            const timestamp = series.processedData[i].timestamp;
                            const belowValue = this._getValueAtTime(nextSeriesBelow.data, timestamp);
                            const x = marginLeft + ((timestamp - minTime) / timeRange) * graphWidth;
                            const y = marginTop + graphHeight - ((belowValue - minValue) / valueRange) * graphHeight;
                            ctx.lineTo(x, y);
                        }
                    } else {
                        const lastTimestamp = series.processedData[series.processedData.length - 1].timestamp;
                        const lastX = marginLeft + ((lastTimestamp - minTime) / timeRange) * graphWidth;
                        ctx.lineTo(lastX, marginTop + graphHeight);
                    }

                    ctx.closePath();
                    ctx.fill();
                }
            }
        } // End fill rendering for line graphs
        
        console.log('DEBUG: After fill rendering block, about to start PHASE 11');

        // === PHASE 11: Draw Line Series (Top Layer) ===
        // Lines drawn in original order, on top of fills
        // Use preprocessed data with gap metadata
        
        console.log('DEBUG: About to check graph_type, value is:', graph_type, 'checking if === 2:', graph_type === 2);
        
        if (graph_type === 2) {
            console.log('DEBUG: ENTERED distribution block');
            // === DISTRIBUTION/HISTOGRAM MODE ===
            const numBins = data.distribution_bins !== undefined ? data.distribution_bins : 10;
            const barDisplayMode = data.bar_display_mode !== undefined ? data.bar_display_mode : 1; // Default to stacked for distribution
            
            console.log('Distribution mode - numBins:', numBins, 'barDisplayMode:', barDisplayMode);
            
            // First pass: find global min/max across all series
            let globalMin = null;
            let globalMax = null;
            
            data.series.forEach(series => {
                if (!series.data || series.data.length === 0) return;
                series.data.forEach(point => {
                    const value = point[1];
                    if (value !== null && value !== undefined && !isNaN(value)) {
                        if (globalMin === null || value < globalMin) globalMin = value;
                        if (globalMax === null || value > globalMax) globalMax = value;
                    }
                });
            });
            
            console.log('Distribution mode - globalMin:', globalMin, 'globalMax:', globalMax);
            
            if (globalMin === null || globalMax === null) return; // No data to display
            
            // Handle edge case: all values are the same
            if (globalMin === globalMax) {
                const singleValue = globalMin;
                
                // Count values per series
                const seriesCounts = data.series.map(series => {
                    if (!series.data || series.data.length === 0) return 0;
                    return series.data.filter(p => p[1] !== null && p[1] !== undefined && !isNaN(p[1])).length;
                });
                
                const totalCount = seriesCounts.reduce((sum, c) => sum + c, 0);
                
                if (barDisplayMode === 1) {
                    // Stacked mode
                    let stackBottom = marginTop + graphHeight;
                    data.series.forEach((series, idx) => {
                        if (seriesCounts[idx] === 0) return;
                        const segmentHeight = (seriesCounts[idx] / totalCount) * graphHeight;
                        const segmentTop = stackBottom - segmentHeight;
                        ctx.fillStyle = series.color;
                        ctx.fillRect(marginLeft, segmentTop, graphWidth, segmentHeight);
                        stackBottom = segmentTop;
                    });
                } else {
                    // Grouped mode
                    const barWidth = graphWidth / data.series.length;
                    data.series.forEach((series, idx) => {
                        if (seriesCounts[idx] === 0) return;
                        const barX = marginLeft + (idx * barWidth);
                        const barHeight = (seriesCounts[idx] / totalCount) * graphHeight;
                        const barY = marginTop + graphHeight - barHeight;
                        ctx.fillStyle = series.color;
                        ctx.fillRect(barX, barY, barWidth * 0.9, barHeight);
                    });
                }
                
                // Draw axis labels
                ctx.fillStyle = yAxisColor;
                ctx.font = '10px Arial';
                ctx.textAlign = 'right';
                ctx.textBaseline = 'middle';
                ctx.fillText(totalCount.toString(), marginLeft - 5, marginTop + graphHeight / 2);
                
                ctx.fillStyle = xAxisColor;
                ctx.textAlign = 'center';
                ctx.textBaseline = 'top';
                ctx.fillText(singleValue.toFixed(2), marginLeft + graphWidth / 2, marginTop + graphHeight + 5);
                
            } else {
                // Normal case: range of values
                const binWidth = (globalMax - globalMin) / numBins;
            
                // Initialize bins - each bin tracks counts per series
                const bins = [];
                for (let i = 0; i < numBins; i++) {
                    const binStart = globalMin + (i * binWidth);
                    const binEnd = globalMin + ((i + 1) * binWidth);
                    bins.push({
                        start: binStart,
                        end: binEnd,
                        seriesCounts: new Array(data.series.length).fill(0), // Count per series
                        totalCount: 0,
                        label: `${binStart.toFixed(2)}-${binEnd.toFixed(2)}`
                    });
                }
                
                // Count values in each bin per series
                data.series.forEach((series, seriesIdx) => {
                    if (!series.data || series.data.length === 0) return;
                    
                    series.data.forEach(point => {
                        const value = point[1];
                        if (value === null || value === undefined || isNaN(value)) return;
                        
                        let binIndex = Math.floor((value - globalMin) / binWidth);
                        if (binIndex >= numBins) binIndex = numBins - 1;
                        if (binIndex < 0) binIndex = 0;
                        
                        bins[binIndex].seriesCounts[seriesIdx]++;
                        bins[binIndex].totalCount++;
                    });
                });
                
                // Find max count for Y-axis scaling
                let maxCount = 0;
                bins.forEach(bin => {
                    if (barDisplayMode === 1) {
                        // Stacked: use total count
                        if (bin.totalCount > maxCount) maxCount = bin.totalCount;
                    } else {
                        // Grouped: use max individual series count
                        bin.seriesCounts.forEach(count => {
                            if (count > maxCount) maxCount = count;
                        });
                    }
                });
                
                if (maxCount === 0) return; // No data
                
                // Store bin data for hover interaction
                this._distributionBins = bins;
                this._distributionMaxCount = maxCount;
                
                // Draw histogram bars
                const binPixelWidth = graphWidth / numBins;
                
                if (barDisplayMode === 1) {
                    // === STACKED MODE ===
                    bins.forEach((bin, binIdx) => {
                        if (bin.totalCount === 0) return;
                        
                        const binX = marginLeft + (binIdx * binPixelWidth);
                        const barSeparation = Math.max(1, binPixelWidth * 0.1);
                        const effectiveBarWidth = binPixelWidth - barSeparation;
                        
                        let stackBottom = marginTop + graphHeight;
                        
                        // Draw each series segment from bottom to top
                        data.series.forEach((series, seriesIdx) => {
                            const count = bin.seriesCounts[seriesIdx];
                            if (count === 0) return;
                            
                            const segmentHeight = (count / maxCount) * graphHeight;
                            const segmentTop = stackBottom - segmentHeight;
                            
                            ctx.fillStyle = series.color;
                            ctx.fillRect(binX + (barSeparation / 2), segmentTop, effectiveBarWidth, segmentHeight);
                            
                            stackBottom = segmentTop;
                        });
                    });
                    
                } else {
                    // === GROUPED MODE (side-by-side) ===
                    const barSeparation = data.bar_separation !== undefined ? data.bar_separation : 2;
                    const numSeries = data.series.length;
                    const groupWidth = binPixelWidth * 0.9; // Use 90% of bin width for the group
                    const individualBarWidth = (groupWidth - (barSeparation * (numSeries - 1))) / numSeries;
                    
                    bins.forEach((bin, binIdx) => {
                        const binCenterX = marginLeft + (binIdx * binPixelWidth) + (binPixelWidth / 2);
                        const groupStartX = binCenterX - (groupWidth / 2);
                        
                        data.series.forEach((series, seriesIdx) => {
                            const count = bin.seriesCounts[seriesIdx];
                            if (count === 0) return;
                            
                            const barX = groupStartX + (seriesIdx * (individualBarWidth + barSeparation));
                            const barHeight = (count / maxCount) * graphHeight;
                            const barY = marginTop + graphHeight - barHeight;
                            
                            ctx.fillStyle = series.color;
                            ctx.fillRect(barX, barY, individualBarWidth, barHeight);
                        });
                    });
                }
                
                // Draw Y-axis labels (counts)
                const countSteps = 5;
                const countStep = maxCount / countSteps;
                
                ctx.fillStyle = yAxisColor;
                ctx.font = '10px Arial';
                ctx.textAlign = 'right';
                ctx.textBaseline = 'middle';
                
                for (let i = 0; i <= countSteps; i++) {
                    const count = Math.round(i * countStep);
                    const y = marginTop + graphHeight - (i * (graphHeight / countSteps));
                    ctx.fillText(count.toString(), marginLeft - 5, y);
                }
                
                // Draw X-axis labels (bin ranges)
                ctx.fillStyle = xAxisColor;
                ctx.textAlign = 'center';
                ctx.textBaseline = 'top';
                
                const maxLabels = 8;
                const labelStep = Math.max(1, Math.ceil(numBins / maxLabels));
                
                bins.forEach((bin, i) => {
                    if (i % labelStep === 0 || i === numBins - 1) {
                        const x = marginLeft + (i * binPixelWidth) + (binPixelWidth / 2);
                        let label;
                        if (Math.abs(bin.start) < 10 && Math.abs(bin.end) < 10) {
                            label = `${bin.start.toFixed(1)}-${bin.end.toFixed(1)}`;
                        } else if (Math.abs(bin.start) < 1000) {
                            label = `${bin.start.toFixed(0)}-${bin.end.toFixed(0)}`;
                        } else {
                            label = `${(bin.start/1000).toFixed(1)}k-${(bin.end/1000).toFixed(1)}k`;
                        }
                        ctx.fillText(label, x, marginTop + graphHeight + 5);
                    }
                });
            } // End else block for normal distribution case
            
        } else if (graph_type === 1) {
            // === BAR CHART MODE ===
            const barSeparation = data.bar_separation !== undefined ? data.bar_separation : 5; // User-defined separation in pixels
            const barDisplayMode = data.bar_display_mode !== undefined ? data.bar_display_mode : 0; // 0=grouped, 1=stacked
            
            if (barDisplayMode === 1) {
                // === STACKED BAR MODE ===
                // Build a map of timestamp -> array of values (one per series)
                const timestampMap = new Map();
                
                data.series.forEach((series, seriesIdx) => {
                    if (!series.processedData || series.processedData.length === 0) return;
                    
                    series.processedData.forEach(point => {
                        if (point.isVirtualZero && missing_data !== 2) return; // Skip virtual zeros unless "treat as 0"
                        
                        if (!timestampMap.has(point.timestamp)) {
                            timestampMap.set(point.timestamp, []);
                        }
                        timestampMap.get(point.timestamp).push({
                            seriesIdx: seriesIdx,
                            value: point.drawValue,
                            color: series.color
                        });
                    });
                });
                
                // Calculate bar width
                const totalPoints = timestampMap.size;
                const barWidth = Math.max(2, Math.min(graphWidth / (totalPoints * 1.5), 40));
                
                // Draw stacked bars
                timestampMap.forEach((stackItems, timestamp) => {
                    const centerX = marginLeft + ((timestamp - minTime) / timeRange) * graphWidth;
                    const barX = centerX - (barWidth / 2);
                    
                    let stackBottom = marginTop + graphHeight;
                    
                    // Draw each segment from bottom to top
                    stackItems.forEach(item => {
                        const value = item.value;
                        const segmentHeight = ((value - minValue) / valueRange) * graphHeight;
                        const segmentTop = stackBottom - segmentHeight;
                        
                        ctx.fillStyle = item.color;
                        ctx.fillRect(barX, segmentTop, barWidth, segmentHeight);
                        
                        stackBottom = segmentTop; // Next segment starts where this one ended
                    });
                });
                
            } else {
                // === GROUPED BAR MODE (side-by-side) ===
                const totalPoints = data.series.reduce((sum, s) => sum + (s.processedData ? s.processedData.length : 0), 0);
                const avgPointsPerSeries = totalPoints / (data.series.length || 1);
                const barWidth = Math.max(2, Math.min(graphWidth / (avgPointsPerSeries * 1.5), 40));
                const groupWidth = (barWidth * data.series.length) + (barSeparation * (data.series.length - 1));
                
                data.series.forEach((series, seriesIdx) => {
                    if (!series.processedData || series.processedData.length === 0) return;

                    ctx.fillStyle = series.color;
                    
                    series.processedData.forEach((point, idx) => {
                        if (point.isVirtualZero && missing_data !== 2) return; // Skip virtual zeros unless "treat as 0"
                        
                        const timestamp = point.timestamp;
                        const value = point.drawValue;
                        
                        // Calculate bar position
                        const centerX = marginLeft + ((timestamp - minTime) / timeRange) * graphWidth;
                        const barX = centerX - (groupWidth / 2) + (seriesIdx * (barWidth + barSeparation));
                        const barTop = marginTop + graphHeight - ((value - minValue) / valueRange) * graphHeight;
                        const barBottom = marginTop + graphHeight;
                        const barHeight = barBottom - barTop;
                        
                        // Draw bar
                        ctx.fillRect(barX, barTop, barWidth, barHeight);
                    });
                });
            }
            
        } else {
            // === LINE CHART MODE (Default) ===
            data.series.forEach(series => {
                if (!series.processedData || series.processedData.length === 0) return;

                ctx.strokeStyle = series.color;
                ctx.lineWidth = 2;
                ctx.beginPath();
                
                if (missing_data === 0) {
                    // None mode: Break line at gaps
                    let inSegment = false;
                    
                    series.processedData.forEach((point, idx) => {
                        const x = marginLeft + ((point.timestamp - minTime) / timeRange) * graphWidth;
                        const y = marginTop + graphHeight - ((point.drawValue - minValue) / valueRange) * graphHeight;

                        if (point.hasGapBefore) {
                            ctx.stroke(); // End current segment
                            ctx.beginPath();
                            inSegment = false;
                        }
                        
                        if (!inSegment) {
                            ctx.moveTo(x, y);
                            inSegment = true;
                        } else {
                            ctx.lineTo(x, y);
                        }
                    });
                    ctx.stroke();
                } else {
                    // Connected or Treat as 0 - draw all processed points
                    series.processedData.forEach((point, idx) => {
                        const x = marginLeft + ((point.timestamp - minTime) / timeRange) * graphWidth;
                        const y = marginTop + graphHeight - ((point.drawValue - minValue) / valueRange) * graphHeight;

                        if (idx === 0) {
                            ctx.moveTo(x, y);
                        } else {
                            ctx.lineTo(x, y);
                        }
                    });
                    ctx.stroke();
                }
            });
        }

        // === PHASE 12: Draw Legend ===
        // Three layout options: top-left (overlays graph), right (sidebar), bottom (below graph)
        const legendColor = data.text_color_legend || '#000';
        ctx.font = '12px Arial';
        ctx.textBaseline = 'middle';

        switch (legend_position) {
            case 0: // Top-left: Overlays graph at (10, 10)
                let legendY = 10;
                data.series.forEach(series => {
                    ctx.fillStyle = series.color;
                    ctx.fillRect(10, legendY, 15, 15); // Color box
                    ctx.fillStyle = legendColor;
                    ctx.textAlign = 'left';
                    ctx.fillText(series.name, 30, legendY + 7); // Series name
                    legendY += 20; // Move to next line
                });
                break;

            case 1: // Right sidebar: Vertical list next to graph
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

            case 2: // Bottom: 4 columns below graph
                const bottomLegendY = height - legendHeight + 10;
                let bottomLegendX = 10;
                const itemsPerRow = 4;
                data.series.forEach((series, idx) => {
                    if (idx > 0 && idx % itemsPerRow === 0) {
                        bottomLegendX = 10; // Reset to left for new row
                    }
                    ctx.fillStyle = series.color;
                    ctx.fillRect(bottomLegendX, bottomLegendY + Math.floor(idx / itemsPerRow) * 20, 15, 15);
                    ctx.fillStyle = legendColor;
                    ctx.textAlign = 'left';
                    ctx.fillText(series.name, bottomLegendX + 20, bottomLegendY + Math.floor(idx / itemsPerRow) * 20 + 7);
                    bottomLegendX += width / itemsPerRow; // Move to next column
                });
                break;
        }

        // === PHASE 13: Setup Interactive Hover Tooltips ===
        // Create or reuse tooltip div element
        let tooltipDiv = container.querySelector('.multigraph-tooltip');
        if (!tooltipDiv) {
            tooltipDiv = document.createElement('div');
            tooltipDiv.className = 'multigraph-tooltip';
            // Styled tooltip: dark background, positioned absolutely, no pointer events (passthrough)
            tooltipDiv.style.cssText = 'position:absolute;background:rgba(0,0,0,0.8);color:#fff;padding:8px;border-radius:4px;font-size:12px;pointer-events:none;display:none;z-index:1000;white-space:nowrap;';
            container.appendChild(tooltipDiv);
        }

        // Overlay context was already created above in PHASE 2

        // Store layout bounds in closures for event handlers
        const graphBounds = { marginLeft, marginTop, graphWidth, graphHeight };
        const timeBounds = { minTime, maxTime, timeRange };
        const valueBounds = { minValue, maxValue, valueRange };

        // Remove old event listeners (prevents memory leaks on re-render)
        if (this._mousemoveHandler) {
            container.removeEventListener('mousemove', this._mousemoveHandler);
            container.removeEventListener('mouseleave', this._mouseleaveHandler);
        }

        // === Mouse Move Handler: Show Values on Hover ===
        this._mousemoveHandler = (e) => {
            const rect = container.getBoundingClientRect();
            const mouseX = e.clientX - rect.left; // Mouse X in canvas coordinates
            const mouseY = e.clientY - rect.top;  // Mouse Y in canvas coordinates

            // Clear previous hover highlights
            overlayCtx.clearRect(0, 0, overlay.width, overlay.height);

            // Hide tooltip if mouse outside graph area
            if (mouseX < graphBounds.marginLeft || mouseX > graphBounds.marginLeft + graphBounds.graphWidth || 
                mouseY < graphBounds.marginTop || mouseY > graphBounds.marginTop + graphBounds.graphHeight) {
                tooltipDiv.style.display = 'none';
                return;
            }

            // === DISTRIBUTION MODE: Highlight bins ===
            if (graph_type === 2 && this._distributionBins) {
                let tooltipHTML = '';
                let hasData = false;

                // Find which bin the mouse is over
                this._distributionBins.forEach(bin => {
                    if (!bin.x) return; // Skip bins with no data
                    
                    if (mouseX >= bin.x && mouseX <= bin.x + bin.width &&
                        mouseY >= bin.y && mouseY <= bin.y + bin.height) {
                        
                        // Highlight the bin with a border
                        overlayCtx.strokeStyle = data.series[0]?.color || '#1f77b4';
                        overlayCtx.lineWidth = 2;
                        overlayCtx.strokeRect(bin.x, bin.y, bin.width, bin.height);
                        
                        // Show tooltip with bin range and count
                        tooltipHTML = `<div><strong>Range:</strong> ${bin.start.toFixed(2)} - ${bin.end.toFixed(2)}</div>`;
                        tooltipHTML += `<div><strong>Count:</strong> ${bin.count}</div>`;
                        hasData = true;
                    }
                });

                if (hasData) {
                    tooltipDiv.innerHTML = tooltipHTML;
                    tooltipDiv.style.display = 'block';
                    
                    let tooltipX = mouseX + 15;
                    let tooltipY = mouseY - 10;
                    
                    const tooltipRect = tooltipDiv.getBoundingClientRect();
                    if (tooltipX + tooltipRect.width > width) {
                        tooltipX = mouseX - tooltipRect.width - 15;
                    }
                    
                    tooltipDiv.style.left = tooltipX + 'px';
                    tooltipDiv.style.top = tooltipY + 'px';
                } else {
                    tooltipDiv.style.display = 'none';
                }
                return;
            }

            // === TIME-SERIES MODE: Highlight data points ===
            // Calculate target time from mouse X position
            const relativeX = (mouseX - graphBounds.marginLeft) / graphBounds.graphWidth;
            const targetTime = timeBounds.minTime + relativeX * timeBounds.timeRange;
            let tooltipHTML = '';
            let hasData = false;

            // For each series, find closest point to target time
            data.series.forEach(series => {
                if (!series.data || series.data.length === 0) return;

                // Find data point closest to mouse X position (by timestamp)
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

                // Draw highlight circle on overlay canvas at data point
                // Drawing in CSS coordinates (overlayCtx has scale transform applied)
                const pointX = graphBounds.marginLeft + ((timestamp - timeBounds.minTime) / timeBounds.timeRange) * graphBounds.graphWidth;
                const pointY = graphBounds.marginTop + graphBounds.graphHeight - ((value - valueBounds.minValue) / valueBounds.valueRange) * graphBounds.graphHeight;

                // Draw hollow circle in series color (no fill, slightly smaller, thinner)
                overlayCtx.strokeStyle = series.color;
                overlayCtx.lineWidth = 1.5;
                overlayCtx.beginPath();
                overlayCtx.arc(pointX, pointY, 4, 0, Math.PI * 2); // 4px radius (was 6)
                overlayCtx.stroke();

                // Build tooltip HTML with colored bullet, series name, value, and time
                const time = new Date(timestamp).toLocaleTimeString();
                tooltipHTML += `<div><span style="color:${series.color}">●</span> ${series.name}: ${value.toFixed(2)} (${time})</div>`;
                hasData = true;
            });

            if (hasData) {
                // Display and position tooltip
                tooltipDiv.innerHTML = tooltipHTML;
                tooltipDiv.style.display = 'block';
                
                // Smart positioning: avoid going off right edge
                let tooltipX = mouseX + 15; // Offset from cursor
                let tooltipY = mouseY - 10;
                
                const tooltipRect = tooltipDiv.getBoundingClientRect();
                if (tooltipX + tooltipRect.width > width) {
                    tooltipX = mouseX - tooltipRect.width - 15; // Flip to left side
                }
                
                tooltipDiv.style.left = tooltipX + 'px';
                tooltipDiv.style.top = tooltipY + 'px';
            } else {
                tooltipDiv.style.display = 'none';
            }
        };

        // === Mouse Leave Handler: Clear Highlights ===
        this._mouseleaveHandler = () => {
            overlayCtx.clearRect(0, 0, overlay.width, overlay.height);
            tooltipDiv.style.display = 'none';
        };

        // Attach event listeners to container div (not overlay canvas)
        container.addEventListener('mousemove', this._mousemoveHandler);
        container.addEventListener('mouseleave', this._mouseleaveHandler);
    }

    /**
     * Helper: Get interpolated value at specific timestamp (duplicate for stacking)
     * 
     * PURPOSE:
     * Finds interpolated value at a specific timestamp for stacked area fills.
     * This is a duplicate of the earlier method for internal use within renderGraph().
     * 
     * ALGORITHM:
     * 1. Handle edge cases (empty data, targetTime before/after data range)
     * 2. Find surrounding data points
     * 3. Check for exact matches
     * 4. Perform linear interpolation between surrounding points
     * 
     * DEPENDENCIES:
     * - Called by: renderGraph() when drawing stacked area fills
     * 
     * @param {Array<Array<number>>} dataPoints Time series data [[timestamp_ms, value], ...]
     * @param {number} targetTime Target timestamp in milliseconds
     * 
     * @returns {number} Interpolated value (or 0 if no data)
     * 
     * @private
     */
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

        // If exact match found
        if (before[0] === targetTime) return before[1];
        if (after[0] === targetTime) return after[1];

        // Linear interpolation: value = v1 + ratio * (v2 - v1)
        const timeDiff = after[0] - before[0];
        if (timeDiff === 0) return before[1]; // Avoid division by zero

        const ratio = (targetTime - before[0]) / timeDiff;
        return before[1] + ratio * (after[1] - before[1]);
    }
}

