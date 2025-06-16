// 轮播图功能
document.addEventListener('DOMContentLoaded', function() {
    const slides = document.querySelector('.carousel-slides');
    const slidesCount = document.querySelectorAll('.carousel-slide').length;
    const prevBtn = document.querySelector('.carousel-prev');
    const nextBtn = document.querySelector('.carousel-next');
    const dots = document.querySelectorAll('.carousel-dot');
    
    let currentIndex = 0;
    
    // 下一张幻灯片
    function nextSlide() {
        currentIndex = (currentIndex + 1) % slidesCount;
        updateCarousel();
    }
    
    // 上一张幻灯片
    function prevSlide() {
        currentIndex = (currentIndex - 1 + slidesCount) % slidesCount;
        updateCarousel();
    }
    
    // 更新轮播图
    function updateCarousel() {
        slides.style.transform = `translateX(-${currentIndex * 100}%)`;
        
        // 更新指示点
        dots.forEach((dot, index) => {
            dot.classList.toggle('active', index === currentIndex);
        });
    }
    
    // 点击指示点切换
    dots.forEach((dot, index) => {
        dot.addEventListener('click', () => {
            currentIndex = index;
            updateCarousel();
        });
    });
    
    // 添加按钮事件监听
    prevBtn.addEventListener('click', prevSlide);
    nextBtn.addEventListener('click', nextSlide);
    
    // 自动轮播
    setInterval(nextSlide, 5000);
    
    // 平滑滚动
    document.querySelectorAll('a[href^="#"]').forEach(anchor => {
        anchor.addEventListener('click', function (e) {
            e.preventDefault();
            const target = document.querySelector(this.getAttribute('href'));
            if (target) {
                target.scrollIntoView({
                    behavior: 'smooth',
                    block: 'start'
                });
            }
        });
    });
    
    // 设置轮播图高度为宽度的75%，实现4:3宽高比
    function adjustCarouselHeight() {
        const carousel = document.querySelector('.carousel-container');
        const slides = document.querySelector('.carousel-slides');
        if (carousel && slides) {
            const width = carousel.clientWidth;
            slides.style.height = (width * 4 /3) + 'px';
        }
    }
    
    // 初始调整
    adjustCarouselHeight();
    
    // 窗口大小变化时重新调整
    window.addEventListener('resize', adjustCarouselHeight);
});

