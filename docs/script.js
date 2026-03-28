// Модуль управления вкладками и Mermaid
class TabManager {
    constructor() {
        this.currentTab = 'modules';
        this.init();
    }

    init() {
        // Инициализация Mermaid
        mermaid.initialize({ 
            startOnLoad: true, 
            securityLevel: 'loose', 
            theme: 'default' 
        });
        
        // Устанавливаем начальную вкладку
        this.showTab('modules');
    }

    showTab(tabName) {
        // Скрываем все вкладки
        const tabs = document.getElementsByClassName('tab-pane');
        for (let i = 0; i < tabs.length; i++) {
            tabs[i].classList.remove('active');
        }
        
        // Убираем активный класс у кнопок
        const buttons = document.getElementsByClassName('nav-tab');
        for (let i = 0; i < buttons.length; i++) {
            buttons[i].classList.remove('active');
        }
        
        // Показываем выбранную вкладку
        const targetTab = document.getElementById(tabName);
        if (targetTab) {
            targetTab.classList.add('active');
            
            // Инициализируем Mermaid для новой видимой вкладки
            this.initMermaidForTab(targetTab);
        }
        
        this.currentTab = tabName;
    }

    initMermaidForTab(tabElement) {
        const mermaidElements = tabElement.querySelectorAll('.mermaid');
        if (mermaidElements.length > 0) {
            mermaid.run({
                nodes: mermaidElements
            }).catch(err => console.error("Mermaid run error:", err));
        }
    }

    // Обработчик клика по кнопке
    handleTabClick(event, tabName) {
        this.showTab(tabName);
        
        // Активируем кнопку
        if (event && event.currentTarget) {
            event.currentTarget.classList.add('active');
        }
    }
}

// Глобальная функция для onclick в HTML
let tabManager;

function showTab(event, tabName) {
    if (tabManager) {
        tabManager.handleTabClick(event, tabName);
    }
}

// Инициализация при загрузке страницы
document.addEventListener('DOMContentLoaded', function() {
    tabManager = new TabManager();
});
