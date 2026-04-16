/**
 * @file mermaid-height-util.js
 * @brief Utility for Mermaid diagram height adjustment and zoom controls
 */

class MermaidHeightAdjuster {
    constructor() {
        this.currentScale = 100;
        this.minScale = 50;
        this.maxScale = 200;
        this.scaleStep = 10;
        this.currentContainer = null;
    }

    /**
     * Initialize Mermaid and setup tab switching
     */
    async initialize() {
        await mermaid.initialize({
            startOnLoad: false,
            theme: 'default',
            themeVariables: {
                primaryColor: '#667eea',
                primaryTextColor: '#333',
                lineColor: '#667eea',
                secondaryColor: '#764ba2',
                fontSize: '14px'
            },
            flowchart: { useMaxWidth: false, htmlLabels: true }
        });

        this.setupTabSwitching();
        this.setupScaleControls();

        // Render first tab
        const firstTab = document.querySelector('.nav-tab.active');
        if (firstTab) {
            this.renderTab(firstTab);
        }
    }

    /**
     * Setup tab switching handlers
     */
    setupTabSwitching() {
        const tabs = document.querySelectorAll('.nav-tab');
        tabs.forEach(tab => {
            tab.addEventListener('click', (e) => {
                this.switchTab(e.target);
            });
        });
    }

    /**
     * Switch to a specific tab
     */
    switchTab(tabElement) {
        // Remove active class from all tabs
        document.querySelectorAll('.nav-tab').forEach(t => t.classList.remove('active'));
        document.querySelectorAll('.tab-pane').forEach(p => p.classList.remove('active'));

        // Add active class to clicked tab
        tabElement.classList.add('active');
        const tabName = tabElement.getAttribute('data-tab');
        const tabPane = document.getElementById(tabName);
        if (tabPane) {
            tabPane.classList.add('active');
            this.renderTab(tabElement);
        }
    }

    /**
     * Render Mermaid diagram for a tab
     */
    async renderTab(tabElement) {
        const tabName = tabElement.getAttribute('data-tab');
        const tabPane = document.getElementById(tabName);
        if (!tabPane) return;

        const diagramContainer = tabPane.querySelector('.diagram-container');
        if (!diagramContainer) return;

        const mermaidElement = diagramContainer.querySelector('.mermaid');
        if (!mermaidElement) return;

        // Reset scale for new tab
        this.currentScale = 100;
        this.currentContainer = diagramContainer;
        this.updateScaleDisplay(diagramContainer);

        // Render Mermaid diagram with delay for DOM visibility
        setTimeout(async () => {
            try {
                await mermaid.run({
                    nodes: [mermaidElement]
                });
                this.adjustDiagram(diagramContainer);
            } catch (error) {
                console.error('Mermaid render error:', error);
            }
        }, 100);
    }

    /**
     * Setup scale controls using event delegation
     */
    setupScaleControls() {
        document.addEventListener('click', (e) => {
            const scaleBtn = e.target.closest('[data-scale-delta]');
            const resetBtn = e.target.closest('[data-reset-scale]');
            
            if (scaleBtn) {
                const delta = parseInt(scaleBtn.getAttribute('data-scale-delta'));
                this.changeScale(delta, scaleBtn);
            } else if (resetBtn) {
                this.resetScale(resetBtn);
            }
        });
    }

    /**
     * Change diagram scale
     */
    changeScale(delta, buttonElement) {
        const container = buttonElement.closest('.diagram-container');
        if (!container) return;

        this.currentScale = Math.min(
            this.maxScale,
            Math.max(this.minScale, this.currentScale + delta)
        );

        this.applyScale(container);
        this.updateScaleDisplay(container);
    }

    /**
     * Reset diagram scale to 100%
     */
    resetScale(buttonElement) {
        const container = buttonElement.closest('.diagram-container');
        if (!container) return;

        this.currentScale = 100;
        this.applyScale(container);
        this.updateScaleDisplay(container);
    }

    /**
     * Apply scale to diagram
     */
    applyScale(container) {
        const mermaidElement = container.querySelector('.mermaid');
        if (!mermaidElement) return;

        const svgElement = mermaidElement.querySelector('svg');
        if (svgElement) {
            svgElement.style.transform = `scale(${this.currentScale / 100})`;
            svgElement.style.transformOrigin = 'center center';
            svgElement.style.transition = 'transform 0.2s ease';
        }
    }

    /**
     * Update scale display
     */
    updateScaleDisplay(container) {
        const scaleDisplay = container.querySelector('.scale-display');
        if (scaleDisplay) {
            scaleDisplay.textContent = `${this.currentScale}%`;
        }
    }

    /**
     * Adjust diagram container size based on SVG bounding box
     */
    adjustDiagram(container) {
        const mermaidElement = container.querySelector('.mermaid');
        if (!mermaidElement) return;

        const svgElement = mermaidElement.querySelector('svg');
        if (!svgElement) return;

        // Get SVG bounding box
        const bbox = svgElement.getBoundingClientRect();
        
        // Calculate required size with padding
        const padding = 40;
        const requiredWidth = bbox.width + padding * 2;
        const requiredHeight = bbox.height + padding * 2;

        // Update container minimum dimensions
        container.style.minWidth = `${Math.max(600, requiredWidth)}px`;
        container.style.minHeight = `${Math.max(400, requiredHeight)}px`;
    }
}

// Initialize when DOM is ready
document.addEventListener('DOMContentLoaded', async () => {
    const adjuster = new MermaidHeightAdjuster();
    await adjuster.initialize();
});
