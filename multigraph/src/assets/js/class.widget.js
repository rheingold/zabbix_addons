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

        const ctx = canvas.getContext('2d'); // Main canvas rendering context
        const data = this._graph_data;       // Alias for readability

        // === PHASE 2: Layout Calculation ===
        // Set canvas size to match container
        const rect = container.getBoundingClientRect();
        canvas.width = rect.width;
        canvas.height = rect.height;
        overlay.width = rect.width;
        overlay.height = rect.height;

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

        // === PHASE 8: Y-Axis Labels ===
        const yAxisColor = data.text_color_yaxis || '#000';
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

        // === PHASE 9: X-Axis Time Labels (Adaptive Formatting) ===
        const xAxisColor = data.text_color_xaxis || '#000';
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

        // === PHASE 10: Draw Stacked Area Fills ===
        const fill_opacity = parseFloat(data.fill_opacity) || 0;

        // Sort series by maximum value (descending) for stacking order
        // Highest values on top, lowest at bottom
        const sortedSeries = [...data.series].sort((a, b) => {
            const maxA = Math.max(...a.data.map(p => p[1]));
            const maxB = Math.max(...b.data.map(p => p[1]));
            return maxB - maxA; // Descending order
        });

        // Draw fills from bottom to top (reverse iteration)
        // Each series fills the area between itself and the series below
        for (let seriesIdx = sortedSeries.length - 1; seriesIdx >= 0; seriesIdx--) {
            const series = sortedSeries[seriesIdx];
            if (!series.data || series.data.length === 0) continue;

            if (fill_opacity > 0) {
                // Convert hex color to RGBA with user-specified opacity
                const hexColor = series.color;
                const r = parseInt(hexColor.slice(1, 3), 16);
                const g = parseInt(hexColor.slice(3, 5), 16);
                const b = parseInt(hexColor.slice(5, 7), 16);
                
                ctx.fillStyle = `rgba(${r}, ${g}, ${b}, ${fill_opacity})`;
                ctx.beginPath();

                // Find the next series below for stacking, or use X-axis baseline
                const nextSeriesBelow = seriesIdx < sortedSeries.length - 1 ? sortedSeries[seriesIdx + 1] : null;

                // Calculate starting position
                const firstTimestamp = series.data[0][0];
                const firstValue = series.data[0][1];
                const firstX = marginLeft + ((firstTimestamp - minTime) / timeRange) * graphWidth;
                const firstY = marginTop + graphHeight - ((firstValue - minValue) / valueRange) * graphHeight;

                // Start path from bottom edge (series below or X-axis)
                if (nextSeriesBelow && nextSeriesBelow.data.length > 0) {
                    // Start from interpolated point on series below
                    const belowValue = this._getValueAtTime(nextSeriesBelow.data, firstTimestamp);
                    const belowY = marginTop + graphHeight - ((belowValue - minValue) / valueRange) * graphHeight;
                    ctx.moveTo(firstX, belowY);
                } else {
                    // Start from X-axis baseline (bottom series)
                    ctx.moveTo(firstX, marginTop + graphHeight);
                }

                // Draw top edge of filled area (current series line)
                series.data.forEach(point => {
                    const timestamp = point[0];
                    const value = point[1];
                    const x = marginLeft + ((timestamp - minTime) / timeRange) * graphWidth;
                    const y = marginTop + graphHeight - ((value - minValue) / valueRange) * graphHeight;
                    ctx.lineTo(x, y);
                });

                // Draw back along bottom edge (series below or baseline)
                if (nextSeriesBelow && nextSeriesBelow.data.length > 0) {
                    // Trace backwards along series below (interpolated at each timestamp)
                    for (let i = series.data.length - 1; i >= 0; i--) {
                        const timestamp = series.data[i][0];
                        const belowValue = this._getValueAtTime(nextSeriesBelow.data, timestamp);
                        const x = marginLeft + ((timestamp - minTime) / timeRange) * graphWidth;
                        const y = marginTop + graphHeight - ((belowValue - minValue) / valueRange) * graphHeight;
                        ctx.lineTo(x, y);
                    }
                } else {
                    // Close to baseline (bottom series)
                    const lastTimestamp = series.data[series.data.length - 1][0];
                    const lastX = marginLeft + ((lastTimestamp - minTime) / timeRange) * graphWidth;
                    ctx.lineTo(lastX, marginTop + graphHeight);
                }

                ctx.closePath();
                ctx.fill();
            }
        }

        // === PHASE 11: Draw Line Series (Top Layer) ===
        // Lines drawn in original order, on top of fills
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

        // Configure overlay canvas to match main canvas dimensions
        overlay.width = canvas.width;
        overlay.height = canvas.height;
        overlay.style.position = 'absolute';
        overlay.style.top = '0';
        overlay.style.left = '0';
        overlay.style.pointerEvents = 'auto'; // CRITICAL: Enable mouse events on overlay
        const overlayCtx = overlay.getContext('2d');

        console.log('Multigraph: Setting up hover on overlay', overlay.id, 'size:', overlay.width, 'x', overlay.height);

        // Store layout bounds in closures for event handlers
        const graphBounds = { marginLeft, marginTop, graphWidth, graphHeight };
        const timeBounds = { minTime, maxTime, timeRange };
        const valueBounds = { minValue, maxValue, valueRange };

        // Remove old event listeners (prevents memory leaks on re-render)
        if (this._mousemoveHandler) {
            overlay.removeEventListener('mousemove', this._mousemoveHandler);
            overlay.removeEventListener('mouseleave', this._mouseleaveHandler);
        }

        // === Mouse Move Handler: Show Values on Hover ===
        this._mousemoveHandler = (e) => {
            const rect = overlay.getBoundingClientRect();
            const mouseX = e.clientX - rect.left; // Mouse X in canvas coordinates
            const mouseY = e.clientY - rect.top;  // Mouse Y in canvas coordinates

            console.log('Multigraph: Mouse move at', mouseX, mouseY);

            // Clear previous hover highlights
            overlayCtx.clearRect(0, 0, overlay.width, overlay.height);

            // Hide tooltip if mouse outside graph area
            if (mouseX < graphBounds.marginLeft || mouseX > graphBounds.marginLeft + graphBounds.graphWidth || 
                mouseY < graphBounds.marginTop || mouseY > graphBounds.marginTop + graphBounds.graphHeight) {
                tooltipDiv.style.display = 'none';
                return;
            }

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
                const pointX = graphBounds.marginLeft + ((timestamp - timeBounds.minTime) / timeBounds.timeRange) * graphBounds.graphWidth;
                const pointY = graphBounds.marginTop + graphBounds.graphHeight - ((value - valueBounds.minValue) / valueBounds.valueRange) * graphBounds.graphHeight;

                overlayCtx.fillStyle = series.color;
                overlayCtx.beginPath();
                overlayCtx.arc(pointX, pointY, 5, 0, Math.PI * 2); // 5px radius circle
                overlayCtx.fill();

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

        // Attach event listeners to overlay canvas
        overlay.addEventListener('mousemove', this._mousemoveHandler);
        overlay.addEventListener('mouseleave', this._mouseleaveHandler);

        console.log('Multigraph: Event listeners attached to overlay', overlay.id);
        console.log('Multigraph: Overlay style:', overlay.style.cssText);
        console.log('Multigraph rendered successfully');
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

