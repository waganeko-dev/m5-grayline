// @ts-check
import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';
import { REPO_URL } from './src/site.config.mjs';

export default defineConfig({
  site: 'https://waganeko-dev.github.io',
  base: '/m5-grayline',
  integrations: [
    starlight({
      title: 'm5-grayline',
      description: 'Grayline map firmware for M5Stack Basic',
      defaultLocale: 'root',
      locales: {
        root: { label: '日本語', lang: 'ja' },
        en: { label: 'English', lang: 'en' },
      },
      social: [{ icon: 'github', label: 'GitHub', href: REPO_URL }],
      editLink: { baseUrl: `${REPO_URL}/edit/main/docs/` },
      components: {
        Footer: './src/components/Footer.astro',
      },
      sidebar: [
        { label: 'はじめに', translations: { en: 'Introduction' }, slug: 'index' },
        {
          label: '書き込み',
          translations: { en: 'Installation' },
          items: [
            { label: '書き込み方法', translations: { en: 'Installing the firmware' }, slug: 'install' },
            { label: 'Web Flasher', slug: 'flasher' },
          ],
        },
        { label: '設定 (config.json)', translations: { en: 'Configuration (config.json)' }, slug: 'config' },
        { label: '画面の見方と操作', translations: { en: 'Screens and buttons' }, slug: 'usage' },
        { label: 'トラブルシューティング', translations: { en: 'Troubleshooting' }, slug: 'troubleshooting' },
        { label: 'データ出典とライセンス', translations: { en: 'Data sources and licenses' }, slug: 'credits' },
        { label: '更新履歴', translations: { en: 'Changelog' }, slug: 'changelog' },
      ],
    }),
  ],
});
